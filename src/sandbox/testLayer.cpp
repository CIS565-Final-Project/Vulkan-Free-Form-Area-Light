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
	std::vector<vk::DescriptorSetLayout> const& descripotrSetLayouts)
{
	// Create required shader stages
	auto task_shader = ReadFile("shaders/mesh_flat.task.spv");
	auto mesh_shader = ReadFile("shaders/mesh_flat.mesh.spv");
	auto frag_shader = ReadFile("shaders/mesh_flat.frag.spv");

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

	Mesh mesh;
	//mesh.LoadMeshFromFile("meshes/stanford_bunny.obj");
	//mesh.LoadMeshFromFile("meshes/sphere.obj");
	//mesh.LoadMeshFromFile("meshes/cube.obj");
	mesh.LoadMeshFromFile(m_MeshFile);

	m_Texture = mkU<VK_Texture2D>(*m_Device);

	m_Texture->CreateFromFile(m_TextureFile, { .format = vk::Format::eR8G8B8A8Unorm, .usage = vk::ImageUsageFlagBits::eSampled });
	//m_Texture->CreateFromFile("images/ltc.dds", {.format = vk::Format::eR32G32B32A32Sfloat, .usage = vk::ImageUsageFlagBits::eSampled });

	m_Texture->TransitionLayout(VK_ImageLayout{
			.layout = vk::ImageLayout::eShaderReadOnlyOptimal,
			.accessFlag = vk::AccessFlagBits::eShaderRead,
			.pipelineStage = vk::PipelineStageFlagBits::eFragmentShader,
		});

	std::vector<TriangleInfo> triangles;

	for (Triangle const& tri : mesh.m_Triangles)
	{
		triangles.emplace_back(
			glm::ivec4(tri.pId, tri.materialId),
			glm::ivec4(tri.nId, 0),
			glm::ivec4(tri.uvId, 0)
		);
	}

	m_MeshletInfo = mkU<MeshletInfo>(32, static_cast<uint32_t>(mesh.m_Triangles.size()));

	// Create Buffers
	m_MeshletInfoBuffer = mkU<VK_DeviceBuffer>(*m_Device);
	m_TriangleBuffer = mkU<VK_DeviceBuffer>(*m_Device);
	m_PositionBuffer = mkU<VK_DeviceBuffer>(*m_Device);
	m_NormalBuffer = mkU<VK_DeviceBuffer>(*m_Device);
	m_UVBuffer = mkU<VK_DeviceBuffer>(*m_Device);

	m_MeshletInfoBuffer->CreateFromData(m_MeshletInfo.get(), sizeof(MeshletInfo), vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive);
	m_TriangleBuffer->CreateFromData(triangles.data(), sizeof(TriangleInfo) * triangles.size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
	m_PositionBuffer->CreateFromData(mesh.m_Positions.data(), sizeof(Float) * mesh.m_Positions.size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
	m_NormalBuffer->CreateFromData(mesh.m_Normals.data(), sizeof(Float) * mesh.m_Normals.size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
	m_UVBuffer->CreateFromData(mesh.m_UVs.data(), sizeof(Float) * mesh.m_UVs.size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);

	// Create Descriptors
	m_CamDescriptor = mkU<VK_Descriptor>(*m_Device);
	m_MeshShaderInputDescriptor = mkU<VK_Descriptor>(*m_Device);

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

	m_MeshShaderInputDescriptor->Create({
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eUniformBuffer,
			.stage = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT,
			.bufferInfo = vk::DescriptorBufferInfo{
				.buffer = m_MeshletInfoBuffer->GetBuffer(),
				.offset = 0,
				.range = m_MeshletInfoBuffer->GetSize()
			}
		},
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eStorageBuffer,
			.stage = vk::ShaderStageFlagBits::eMeshEXT,
			.bufferInfo = vk::DescriptorBufferInfo{
				.buffer = m_TriangleBuffer->GetBuffer(),
				.offset = 0,
				.range = m_TriangleBuffer->GetSize()
			}
		},
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eStorageBuffer,
			.stage = vk::ShaderStageFlagBits::eMeshEXT,
			.bufferInfo = vk::DescriptorBufferInfo{
				.buffer = m_PositionBuffer->GetBuffer(),
				.offset = 0,
				.range = m_PositionBuffer->GetSize()
			}
		},
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eStorageBuffer,
			.stage = vk::ShaderStageFlagBits::eMeshEXT,
			.bufferInfo = vk::DescriptorBufferInfo{
				.buffer = m_NormalBuffer->GetBuffer(),
				.offset = 0,
				.range = m_NormalBuffer->GetSize()
			}
		},
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eStorageBuffer,
			.stage = vk::ShaderStageFlagBits::eMeshEXT,
			.bufferInfo = vk::DescriptorBufferInfo{
				.buffer = m_UVBuffer->GetBuffer(),
				.offset = 0,
				.range = m_UVBuffer->GetSize()
			}
		},
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eCombinedImageSampler,
			.stage = vk::ShaderStageFlagBits::eFragment,
			.imageInfo = vk::DescriptorImageInfo{
				.sampler = m_Texture->GetSampler(),
				.imageView = m_Texture->GetImageView(),
				.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
			}
		},
		});

	// Create Pipeline
	m_MeshShaderPipeline = mkU<VK_GraphicsPipeline>(*m_Device);

	std::vector<vk::DescriptorSetLayout> descriptor_set_layouts{
		m_CamDescriptor->GetDescriptorSetLayout(),
		m_MeshShaderInputDescriptor->GetDescriptorSetLayout(),
	};
	CreateMeshPipeline(m_Device->GetDevice(), m_MeshShaderPipeline.get(), descriptor_set_layouts);

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
			m_Camera->m_Transform.RotateAround(glm::vec3(0.f), { 0.1f * offset.y, -offset.x, 0 });
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

		cmd[0].bindPipeline(vk::PipelineBindPoint::eGraphics, m_MeshShaderPipeline->vk_Pipeline);

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

		// Draw call
		uint32_t num_workgroups_x = (m_MeshletInfo->Triangle_Count + m_MeshletInfo->Meshlet_Size - 1) / m_MeshletInfo->Meshlet_Size;
		uint32_t num_workgroups_y = 1;
		uint32_t num_workgroups_z = 1;

		vk::ArrayProxy<vk::DescriptorSet> arr{
			m_CamDescriptor->GetDescriptorSet(),
			m_MeshShaderInputDescriptor->GetDescriptorSet()
		};

		cmd[0].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			m_MeshShaderPipeline->vk_PipelineLayout,
			uint32_t(0),
			arr, nullptr);

		cmd[0].drawMeshTasksEXT(num_workgroups_x, num_workgroups_y, num_workgroups_z);

		cmd.End();
	}
}
