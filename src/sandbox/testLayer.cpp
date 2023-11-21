#include "testLayer.h"

#include <iostream>

#include <SDL.h>

#include "imgui.h"

using namespace VK_Renderer;

struct CameraUBO
{
	glm::mat4 viewProjMat;
};

struct MeshletInfo
{
	uint32_t Meshlet_Size;
	uint32_t Triangle_Count;
};

struct TriangleInfo
{
	glm::ivec4 pId;
	glm::ivec4 nId;
	glm::ivec4 uvId;
};

void CreateMeshPipeline(vk::Device vk_device,
	VK_GraphicsPipeline* pipeline,
	std::vector<vk::DescriptorSetLayout> const& descripotrSetLayouts,
	const std::string& task_shader_path,
	const std::string& mesh_shader_path,
	const std::string& frag_shader_path)
{
	// Create required shader stages
	auto task_shader = ReadFile(task_shader_path);
	auto mesh_shader = ReadFile(mesh_shader_path);
	auto frag_shader = ReadFile(frag_shader_path);

	vk::ShaderModule task_module = vk_device.createShaderModule(vk::ShaderModuleCreateInfo{
		.codeSize = task_shader.size(),
		.pCode = reinterpret_cast<const uint32_t*>(task_shader.data())
		});
	vk::ShaderModule mesh_module = vk_device.createShaderModule(vk::ShaderModuleCreateInfo{
		.codeSize = mesh_shader.size(),
		.pCode = reinterpret_cast<const uint32_t*>(mesh_shader.data())
		});
	vk::ShaderModule frag_module = vk_device.createShaderModule(vk::ShaderModuleCreateInfo{
		.codeSize = frag_shader.size(),
		.pCode = reinterpret_cast<const uint32_t*>(frag_shader.data())
		});

	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages
	{
		vk::PipelineShaderStageCreateInfo{
			.stage = vk::ShaderStageFlagBits::eTaskEXT,
			.module = task_module,
			.pName = "main"
		},
		vk::PipelineShaderStageCreateInfo{
			.stage = vk::ShaderStageFlagBits::eMeshEXT,
			.module = mesh_module,
			.pName = "main"
		},
		vk::PipelineShaderStageCreateInfo{
			.stage = vk::ShaderStageFlagBits::eFragment,
			.module = frag_module,
			.pName = "main"
		},
	};

	VK_PipelineInput input;
	input.SetupPipelineVertexInputCreateInfo();

	pipeline->CreatePipeline({.renderPass = Application::GetInstance()->GetRenderEngine()->GetRenderPass()->GetRenderPass()}, shader_stages, input, descripotrSetLayouts);

	vk_device.destroyShaderModule(task_module);
	vk_device.destroyShaderModule(mesh_module);
	vk_device.destroyShaderModule(frag_module);
}

RenderLayer::RenderLayer(std::string const& name, 
							std::string const&& meshFile,
							std::string const&& textureFile)
	: Layer(name), m_MeshFile(meshFile), m_TextureFile(textureFile)
{
}

void RenderLayer::OnAttach()
{
	m_Camera = mkU<PerspectiveCamera>();
	m_Camera->m_Transform = Transformation{
		.position = {0, 0, -10}
	};
	m_Camera->resolution = { 680, 680 };
	m_Camera->RecomputeProjView();

	m_Engine = Application::GetInstance()->GetRenderEngine();
	m_Device = m_Engine->GetDevice();
	m_Swapchain = m_Engine->GetSwapchain();

	m_Cmd = mkU<VK_CommandBuffer>(m_Device->GetGraphicsCommandPool()->AllocateCommandBuffers({ .level = vk::CommandBufferLevel::eSecondary }));

	// Create Camera buffer
	m_CamBuffer = mkU<VK_StagingBuffer>(*m_Device);
	glm::mat4 vp = m_Camera->GetProjViewMatrix();
	m_CamBuffer->CreateFromData(&vp, sizeof(CameraUBO), vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive);

	Mesh ltcMesh;
	//mesh.LoadMeshFromFile("meshes/stanford_bunny.obj");
	//mesh.LoadMeshFromFile("meshes/sphere.obj");
	//mesh.LoadMeshFromFile("meshes/cube.obj");
	ltcMesh.LoadMeshFromFile(m_MeshFile);

	Mesh lightMesh;
	lightMesh.LoadMeshFromFile("meshes/stanford_bunny.obj");

	m_Texture = mkU<VK_Texture2D>(*m_Device);

	m_Texture->CreateFromFile(m_TextureFile, { .format = vk::Format::eR8G8B8A8Unorm, .usage = vk::ImageUsageFlagBits::eSampled });
	//m_Texture->CreateFromFile("images/ltc.dds", {.format = vk::Format::eR32G32B32A32Sfloat, .usage = vk::ImageUsageFlagBits::eSampled });

	m_Texture->TransitionLayout(VK_ImageLayout{
			.layout = vk::ImageLayout::eShaderReadOnlyOptimal,
			.accessFlag = vk::AccessFlagBits::eShaderRead,
			.pipelineStage = vk::PipelineStageFlagBits::eFragmentShader,
		});

	std::vector<TriangleInfo> ltcTriangles;

	for (Triangle const& tri : ltcMesh.m_Triangles)
	{
		ltcTriangles.emplace_back(
			glm::ivec4(tri.pId, tri.materialId),
			glm::ivec4(tri.nId, 0),
			glm::ivec4(tri.uvId, 0)
		);
	}

	std::vector<TriangleInfo> lightTriangles;

	for (Triangle const& tri : lightMesh.m_Triangles)
	{
		lightTriangles.emplace_back(
			glm::ivec4(tri.pId, tri.materialId),
			glm::ivec4(tri.nId, 0),
			glm::ivec4(tri.uvId, 0)
		);
	}

	m_LTCMeshletInfo = mkU<MeshletInfo>(32, static_cast<uint32_t>(ltcMesh.m_Triangles.size()));
	m_LightMeshletInfo = mkU<MeshletInfo>(32, static_cast<uint32_t>(lightMesh.m_Triangles.size()));

	// Create Buffers

	auto generateMeshBufferSet = [=](MeshBufferSet& bufferSet, std::vector<TriangleInfo>& tris, Mesh& mesh, uPtr<MeshletInfo>& meshletInfo) -> void {
		bufferSet.m_MeshletInfoBuffer = mkU<VK_DeviceBuffer>(*m_Device);
		bufferSet.m_TriangleBuffer = mkU<VK_DeviceBuffer>(*m_Device);
		bufferSet.m_PositionBuffer = mkU<VK_DeviceBuffer>(*m_Device);
		bufferSet.m_NormalBuffer = mkU<VK_DeviceBuffer>(*m_Device);
		bufferSet.m_UVBuffer = mkU<VK_DeviceBuffer>(*m_Device);

		bufferSet.m_MeshletInfoBuffer->CreateFromData(meshletInfo.get(), sizeof(MeshletInfo), vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive);
		bufferSet.m_TriangleBuffer->CreateFromData(tris.data(), sizeof(TriangleInfo) * tris.size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
		bufferSet.m_PositionBuffer->CreateFromData(mesh.m_Positions.data(), sizeof(Float) * mesh.m_Positions.size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
		bufferSet.m_NormalBuffer->CreateFromData(mesh.m_Normals.data(), sizeof(Float) * mesh.m_Normals.size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
		bufferSet.m_UVBuffer->CreateFromData(mesh.m_UVs.data(), sizeof(Float) * mesh.m_UVs.size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
	};


	generateMeshBufferSet(m_LTCMeshBufferSet, ltcTriangles, ltcMesh, m_LTCMeshletInfo);

	generateMeshBufferSet(m_LightMeshBufferSet, lightTriangles, lightMesh, m_LightMeshletInfo);


	// Create Descriptors
	m_CamDescriptor = mkU<VK_Descriptor>(*m_Device);
	m_LTCMeshShaderInputDescriptor = mkU<VK_Descriptor>(*m_Device);
	m_LightMeshShaderInputDescriptor = mkU<VK_Descriptor>(*m_Device);

	m_CamDescriptor->Create({
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eUniformBuffer,
			.stage = vk::ShaderStageFlagBits::eMeshEXT,
			.bufferInfo = vk::DescriptorBufferInfo{
				.buffer = m_CamBuffer->GetBuffer(),
				.offset = 0,
				.range = m_CamBuffer->GetSize()
			}
		}
	});
	
	auto generateDescriptorBinds = [](MeshBufferSet& bufferSet) -> std::vector<VK_DescriptorBinding> {
		return {
			VK_DescriptorBinding{
				.type = vk::DescriptorType::eUniformBuffer,
				.stage = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = bufferSet.m_MeshletInfoBuffer->GetBuffer(),
					.offset = 0,
					.range = bufferSet.m_MeshletInfoBuffer->GetSize()
				}
			},
			VK_DescriptorBinding{
				.type = vk::DescriptorType::eStorageBuffer,
				.stage = vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = bufferSet.m_TriangleBuffer->GetBuffer(),
					.offset = 0,
					.range = bufferSet.m_TriangleBuffer->GetSize()
				}
			},
			VK_DescriptorBinding{
				.type = vk::DescriptorType::eStorageBuffer,
				.stage = vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = bufferSet.m_PositionBuffer->GetBuffer(),
					.offset = 0,
					.range = bufferSet.m_PositionBuffer->GetSize()
				}
			},
			VK_DescriptorBinding{
				.type = vk::DescriptorType::eStorageBuffer,
				.stage = vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = bufferSet.m_NormalBuffer->GetBuffer(),
					.offset = 0,
					.range = bufferSet.m_NormalBuffer->GetSize()
				}
			},
			VK_DescriptorBinding{
				.type = vk::DescriptorType::eStorageBuffer,
				.stage = vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = bufferSet.m_UVBuffer->GetBuffer(),
					.offset = 0,
					.range = bufferSet.m_UVBuffer->GetSize()
				}
			},
		};
	};

	std::vector<VK_DescriptorBinding> ltcMeshShaderDescriptorSet = generateDescriptorBinds(m_LTCMeshBufferSet);

	ltcMeshShaderDescriptorSet.push_back(VK_DescriptorBinding{
		.type = vk::DescriptorType::eCombinedImageSampler,
		.stage = vk::ShaderStageFlagBits::eFragment,
		.imageInfo = vk::DescriptorImageInfo{
			.sampler = m_Texture->GetSampler(),
			.imageView = m_Texture->GetImageView(),
			.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
		}
	});

	m_LTCMeshShaderInputDescriptor->Create(ltcMeshShaderDescriptorSet);

	std::vector<VK_DescriptorBinding> lightMeshShaderDescriptorSet = generateDescriptorBinds(m_LightMeshBufferSet);

	lightMeshShaderDescriptorSet.push_back(VK_DescriptorBinding{
		.type = vk::DescriptorType::eCombinedImageSampler,
		.stage = vk::ShaderStageFlagBits::eFragment,
		.imageInfo = vk::DescriptorImageInfo{
			.sampler = m_Texture->GetSampler(),
			.imageView = m_Texture->GetImageView(),
			.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
		}
	});

	m_LightMeshShaderInputDescriptor->Create(lightMeshShaderDescriptorSet);

	// TEMP: Try two pipelines with same descriptor set
	// Create Pipeline
	m_MeshShaderLightPipeline = mkU<VK_GraphicsPipeline>(*m_Device);
	m_MeshShaderLTCPipeline = mkU<VK_GraphicsPipeline>(*m_Device);

	std::vector<vk::DescriptorSetLayout> ltc_descriptor_set_layouts{
		m_CamDescriptor->GetDescriptorSetLayout(),
		m_LTCMeshShaderInputDescriptor->GetDescriptorSetLayout(),
	};
	std::vector<vk::DescriptorSetLayout> light_descriptor_set_layouts{
		m_CamDescriptor->GetDescriptorSetLayout(),
		m_LightMeshShaderInputDescriptor->GetDescriptorSetLayout(),
	};
	CreateMeshPipeline(m_Device->GetDevice(), m_MeshShaderLightPipeline.get(), ltc_descriptor_set_layouts,
		"shaders/mesh_flat.task.spv", "shaders/mesh_flat.mesh.spv", "shaders/mesh_flat.frag.spv");
	CreateMeshPipeline(m_Device->GetDevice(), m_MeshShaderLTCPipeline.get(), light_descriptor_set_layouts,
		"shaders/mesh_ltc.task.spv", "shaders/mesh_ltc.mesh.spv", "shaders/mesh_ltc.frag.spv");

	RecordCmd();
	m_Engine->PushSecondaryCommandAll((*m_Cmd)[0]);
}

void RenderLayer::OnDetech()
{
}

void RenderLayer::OnUpdate(double const& deltaTime)
{
	
}

void RenderLayer::OnRender(double const& deltaTime)
{
}

void RenderLayer::OnImGui(double const& deltaTime)
{
	static float v = 0.f;
	ImGui::Begin("Test Window");
	ImGui::DragFloat("Test Value", &v, 0.1f, 0.f, 10.f);
	ImGui::End();
}

bool RenderLayer::OnEvent(SDL_Event const& e)
{
	static glm::ivec2 mouse_pre;
	if (e.type == SDL_MOUSEMOTION)
	{
		const Uint8* state = SDL_GetKeyboardState(nullptr);
		glm::ivec2 mouse_cur;
		SDL_GetMouseState(&mouse_cur.x, &mouse_cur.y);
		Uint32 mouseState = SDL_GetMouseState(nullptr, nullptr);
		if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
		{
			glm::vec2 offset = 0.001f * glm::vec2(mouse_cur - mouse_pre);
			m_Camera->m_Transform.Translate({ -offset.x, offset.y, 0 });
			m_Camera->RecomputeProjView();
			glm::mat4 view_proj_mat = m_Camera->GetProjViewMatrix();

			m_CamBuffer->Update(&view_proj_mat, 0, sizeof(glm::mat4));
		}
		if (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE))
		{
			glm::vec2 offset = 0.01f * glm::vec2(mouse_cur - mouse_pre);
			m_Camera->m_Transform.RotateAround(glm::vec3(0.f), { 0.4f * offset.y, -offset.x, 0 });
			m_Camera->RecomputeProjView();
			glm::mat4 view_proj_mat = m_Camera->GetProjViewMatrix();

			m_CamBuffer->Update(&view_proj_mat, 0, sizeof(glm::mat4));
		}
		mouse_pre = mouse_cur;
	}

	return false;
}

void RenderLayer::RecordCmd()
{
	VK_CommandBuffer& cmd = *m_Cmd;
	cmd.Reset();
	{
		cmd.Begin({ .usage = vk::CommandBufferUsageFlagBits::eRenderPassContinue | vk::CommandBufferUsageFlagBits::eSimultaneousUse,
			.inheritInfo = {
				.renderPass = Application::GetInstance()->GetRenderEngine()->GetRenderPass()->GetRenderPass(),
				.subpass = 0}
			});



		// Viewport and scissors
		cmd[0].setViewport(0, vk::Viewport{
			.x = 0.f,
			.y = 0.f,
			.width = static_cast<float>(m_Swapchain->vk_ImageExtent.width),
			.height = static_cast<float>(m_Swapchain->vk_ImageExtent.height),
			.minDepth = 0.f,
			.maxDepth = 1.f
			});

		cmd[0].setScissor(0, vk::Rect2D{
			.offset = { 0, 0 },
			.extent = m_Swapchain->vk_ImageExtent
		});

		// draw LTC objects
		{
			cmd[0].bindPipeline(vk::PipelineBindPoint::eGraphics, m_MeshShaderLTCPipeline->vk_Pipeline);

			// Draw call
			uint32_t num_workgroups_x = (m_LTCMeshletInfo->Triangle_Count + m_LTCMeshletInfo->Meshlet_Size - 1) / m_LTCMeshletInfo->Meshlet_Size;
			uint32_t num_workgroups_y = 1;
			uint32_t num_workgroups_z = 1;

			std::vector<vk::DescriptorSet> arr{
				m_CamDescriptor->GetDescriptorSet(),
				m_LTCMeshShaderInputDescriptor->GetDescriptorSet()
			};


			cmd[0].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
				m_MeshShaderLTCPipeline->vk_PipelineLayout,
				uint32_t(0),
				arr, nullptr);

			cmd[0].drawMeshTasksEXT(num_workgroups_x, num_workgroups_y, num_workgroups_z);
		}

		// draw Light objects
		{
			cmd[0].bindPipeline(vk::PipelineBindPoint::eGraphics, m_MeshShaderLightPipeline->vk_Pipeline);

			// Draw call
			uint32_t num_workgroups_x = (m_LightMeshletInfo->Triangle_Count + m_LightMeshletInfo->Meshlet_Size - 1) / m_LightMeshletInfo->Meshlet_Size;
			uint32_t num_workgroups_y = 1;
			uint32_t num_workgroups_z = 1;

			std::vector<vk::DescriptorSet> arr{
				m_CamDescriptor->GetDescriptorSet(),
				m_LightMeshShaderInputDescriptor->GetDescriptorSet()
			};


			cmd[0].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
				m_MeshShaderLightPipeline->vk_PipelineLayout,
				uint32_t(0),
				arr, nullptr);

			cmd[0].drawMeshTasksEXT(num_workgroups_x, num_workgroups_y, num_workgroups_z);
		}


		cmd.End();
	}
}
