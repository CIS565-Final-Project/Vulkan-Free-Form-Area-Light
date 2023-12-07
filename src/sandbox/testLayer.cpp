#include "testLayer.h"

#include <iostream>

#include <SDL.h>

#include "imgui.h"

using namespace VK_Renderer;

struct CameraUBO
{
	glm::vec4 pos;
	glm::mat4 viewProjMat;
	std::array<glm::vec4, 6> planes;
};

RenderLayer::RenderLayer(std::string const& name)
	: Layer(name)
{
}

void RenderLayer::OnAttach()
{
	m_Camera = mkU<PerspectiveCamera>();
	//m_Camera->far = 500.f;
	m_Camera->m_Transform = Transformation{
		.position = {0, 0, 15},
	};
	m_Camera->m_Transform.Rotate(glm::pi<float>(), { 0, 1, 0 });
	m_Camera->resolution = { 680, 680 };
	m_Camera->RecomputeProjView();

	m_Engine = Application::GetInstance()->GetRenderEngine();
	m_Device = m_Engine->GetDevice();
	m_Swapchain = m_Engine->GetSwapchain();

	m_Cmd = mkU<VK_CommandBuffer>(m_Device->GetGraphicsCommandPool()->AllocateCommandBuffers({ .level = vk::CommandBufferLevel::eSecondary }));

	// Load Scene
	m_Scene = mkU<Scene>();
	m_SceneLight = mkU<SceneLight>();
	m_SceneLight->SetBlurLayerCount(9);
	LoadScene();

	// Generate textures
	m_LightBlurTexture = mkU<VK_Texture2DArray>(*m_Device);
	m_CompressedTexture = mkU<VK_Texture2DArray>(*m_Device);
	m_DDSTexture = mkU<VK_Texture2D>(*m_Device);
	m_DDSAmpFresnel = mkU<VK_Texture2D>(*m_Device);
	GenTextures();

	// Generate Buffers
	m_CamBuffer = mkU<VK_StagingBuffer>(*m_Device);
	m_MeshletInfoBuffer = mkU<VK_DeviceBuffer>(*m_Device);
	m_VertexIndicesBuffer = mkU<VK_DeviceBuffer>(*m_Device);
	m_PrimitiveIndicesBuffer = mkU<VK_DeviceBuffer>(*m_Device);
	m_VertexBuffer = mkU<VK_DeviceBuffer>(*m_Device);
	m_ModelMatrixBuffer = mkU<VK_StagingBuffer>(*m_Device);
	m_MaterialParamBuffer = mkU<VK_StagingBuffer>(*m_Device);
	m_LightBuffer = mkU<VK_StagingBuffer>(*m_Device);
	m_LightCountBuffer = mkU<VK_StagingBuffer>(*m_Device);
	GenBuffers();

	// Create Descriptors
	m_MaterialParamDescriptor = mkU<VK_Descriptor>(*m_Device);
	m_LightDescriptor = mkU<VK_Descriptor>(*m_Device);
	m_CamDescriptor = mkU<VK_Descriptor>(*m_Device);
	m_LTCMeshShaderInputDescriptor = mkU<VK_Descriptor>(*m_Device);
	CreateDescriptors();

	// Create Pipeline
	m_MeshShaderLightPipeline = mkU<VK_GraphicsPipeline>(*m_Device, *m_Engine->GetRenderPass());
	m_MeshShaderLTCPipeline = mkU<VK_GraphicsPipeline>(*m_Device, *m_Engine->GetRenderPass());
	CreateGraphicsPipeline();

	RecordCmd();
	m_Engine->PushSecondaryCommandAll((*m_Cmd)[0]);
}

void RenderLayer::OnDetech()
{
}

void RenderLayer::OnUpdate(double const& deltaTime)
{
	if (false)
	{
		static float time = 0.f;

		time += deltaTime;
		static int id = 1;
		m_MaterialParamBuffer->Update(&time, 0, sizeof(float));
		AreaLight& area_light = *m_SceneLight->GetLight(id);
		area_light.m_Transform.RotateAround({ 0, 0, 0 }, { 0, deltaTime, 0 });
		LightInfo updatedInfo = area_light.GetLightInfo();
		m_LightBuffer->Update(&updatedInfo, sizeof(LightInfo) * id, sizeof(LightInfo));
	}
}

void RenderLayer::OnRender(double const& deltaTime)
{
}

void RenderLayer::OnImGui(double const& deltaTime)
{
	static glm::vec4 v(1.f);
	ImGui::Begin("Test Window");
	if (ImGui::DragFloat4("MaterialParam", &v[0], 0.02f, 0.f, 1.f))
	{
		m_MaterialParamBuffer->Update(&v, 0, sizeof(glm::vec4));
	}
	if (ImGui::DragFloat("Alpha", &m_Camera->alpha, 0.01f, 0.f, 1.f))
	{
		CameraUBO camera_ubo;
		camera_ubo.pos = glm::vec4(m_Camera.get()->GetTransform().position, 1);
		camera_ubo.viewProjMat = m_Camera->GetProjViewMatrix();
		camera_ubo.planes = m_Camera->GetPlanes();
		m_CamBuffer->Update(&camera_ubo, 0, sizeof(CameraUBO));
	}
	ImGui::End();

	ImGui::Begin("Model Window");
	{
		static uint32_t current_id = 0;

		std::vector<MeshProxy>& meshes = m_Scene->GetMeshProxies();

		if (ImGui::BeginCombo("##combo", meshes[current_id].name.c_str()))
		{
			for (int i = 0; i < meshes.size(); ++i)
			{
				bool is_selected = (current_id == i);
				if (ImGui::Selectable(meshes[i].name.c_str(), is_selected))
				{
					current_id = i;
				}
			}
			ImGui::EndCombo();
		}

		bool modified = false;
		
		MeshProxy& mesh = meshes[current_id];
		glm::vec3 euler = glm::eulerAngles(mesh.transform.rotation);

		modified |= ImGui::DragFloat3("Position", &mesh.transform.position[0], 0.1f, -200.f, 200.f);
		modified |= ImGui::DragFloat3("Rotation", &euler[0], 0.1f, -2.f * glm::pi<float>(), 2.f * glm::pi<float>());
		modified |= ImGui::DragFloat3("Scale", &mesh.transform.scale[0], 0.01f, 0.01f, 1.f);
		
		if (modified)
		{
			mesh.transform.rotation = glm::quat(euler);
			m_Scene->UpdateModelMatrix(current_id);
			m_ModelMatrixBuffer->Update(&m_Scene->GetModelMatries()[mesh.id], sizeof(ModelMatrix) * mesh.id, sizeof(ModelMatrix));
		}
	}
	ImGui::End();

	ImGui::Begin("Light Window");
	{
		static uint32_t current_id = 0;

		std::vector<MeshProxy>& meshes = m_Scene->GetMeshProxies();
		std::vector<std::string> lightIdxs(m_SceneLight->GetLightCount());
		for (auto i = 0; i < m_SceneLight->GetLightCount();++i) {
			lightIdxs[i] = std::to_string(i);
		}
		//assert(lightIdxs.size() > 0);
		if (lightIdxs.size() > 0) {
			if (ImGui::BeginCombo("##combo", lightIdxs[current_id].c_str()))
			{
				for (int i = 0; i < lightIdxs.size(); ++i)
				{
					bool is_selected = (current_id == i);
					if (ImGui::Selectable(lightIdxs[i].c_str(), is_selected))
					{
						current_id = i;
					}
				}
				ImGui::EndCombo();
			}
		}

		bool modified = false;
		AreaLight& area_light = *m_SceneLight->GetLight(current_id);
		glm::vec3 euler = glm::eulerAngles(area_light.m_Transform.rotation);
		modified |= ImGui::DragFloat3("Position", &area_light.m_Transform.position[0], 0.1f, -200.f, 200.f);
		modified |= ImGui::DragFloat("Amplitude", &area_light.m_Amplitude, 0.1f, 0.f, 20.f);
		modified |= ImGui::DragFloat3("Rotation", &euler[0], 0.1f, -2.f * glm::pi<float>(), 2.f * glm::pi<float>());
		modified |= ImGui::DragFloat3("Scale", &area_light.m_Transform.scale[0], 0.1f, 0.1f, 3.f);

		if (modified)
		{
			area_light.m_Transform.rotation = glm::quat(euler);
			LightInfo updatedInfo = area_light.GetLightInfo();
			m_LightBuffer->Update(&updatedInfo, sizeof(LightInfo) * current_id, sizeof(LightInfo));
		}
	}
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

			CameraUBO camera_ubo;
			camera_ubo.pos = glm::vec4(m_Camera.get()->GetTransform().position, 1);
			camera_ubo.viewProjMat = m_Camera->GetProjViewMatrix();
			camera_ubo.planes = m_Camera->GetPlanes();
			m_CamBuffer->Update(&camera_ubo, 0, sizeof(CameraUBO));
		}
		if (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE))
		{
			glm::vec2 offset = 0.01f * glm::vec2(mouse_cur - mouse_pre);
			m_Camera->m_Transform.RotateAround(glm::vec3(0.f), { -offset.y, -offset.x, 0 });
			m_Camera->RecomputeProjView();
			
			CameraUBO camera_ubo;
			camera_ubo.pos = glm::vec4(m_Camera.get()->GetTransform().position, 1);
			camera_ubo.viewProjMat = m_Camera->GetProjViewMatrix();
			camera_ubo.planes = m_Camera->GetPlanes();
			m_CamBuffer->Update(&camera_ubo, 0, sizeof(CameraUBO));
		}
		mouse_pre = mouse_cur;
	}
	if (e.type == SDL_WINDOWEVENT)
	{
		if (e.window.event == SDL_WINDOWEVENT_RESIZED)
		{
			m_Swapchain = m_Engine->GetSwapchain();
			RecordCmd();

			m_Camera->resolution = { e.window.data1, e.window.data2 };

			m_Camera->RecomputeProjView();

			CameraUBO camera_ubo;
			camera_ubo.pos = glm::vec4(m_Camera.get()->GetTransform().position, 1);
			camera_ubo.viewProjMat = m_Camera->GetProjViewMatrix();
			camera_ubo.planes = m_Camera->GetPlanes();
			m_CamBuffer->Update(&camera_ubo, 0, sizeof(CameraUBO));
		}
		if (e.window.event == SDL_WINDOWEVENT_MAXIMIZED)
		{
			m_Swapchain = m_Engine->GetSwapchain();
			RecordCmd();

			int width, height;
			SDL_GetWindowSize(reinterpret_cast<SDL_Window*>(Application::GetInstance()->GetWindow()), &width, &height);
			m_Camera->resolution = { width, height };

			m_Camera->RecomputeProjView();

			CameraUBO camera_ubo;
			camera_ubo.pos = glm::vec4(m_Camera.get()->GetTransform().position, 1);
			camera_ubo.viewProjMat = m_Camera->GetProjViewMatrix();
			camera_ubo.planes = m_Camera->GetPlanes();
			m_CamBuffer->Update(&camera_ubo, 0, sizeof(CameraUBO));
		}
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
			cmd[0].bindPipeline(vk::PipelineBindPoint::eGraphics, m_MeshShaderLTCPipeline->GetPipeline());

			// Draw call
			uint32_t num_workgroups_x = m_Scene->GetMeshlets()->GetMeshletInfos().size();
			uint32_t num_workgroups_y = 1;
			uint32_t num_workgroups_z = 1;

			std::vector<vk::DescriptorSet> arr{
				m_CamDescriptor->GetDescriptorSet(),
				m_LTCMeshShaderInputDescriptor->GetDescriptorSet(),
				m_LightDescriptor->GetDescriptorSet(),
				m_MaterialParamDescriptor->GetDescriptorSet(),
			};


			cmd[0].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
				m_MeshShaderLTCPipeline->GetLayout(),
				uint32_t(0),
				arr, nullptr);

			cmd[0].drawMeshTasksEXT(num_workgroups_x, num_workgroups_y, num_workgroups_z);
		}

		// draw Light objects
		{
			cmd[0].bindPipeline(vk::PipelineBindPoint::eGraphics, m_MeshShaderLightPipeline->GetPipeline());

			// Draw call
			// uint32_t num_workgroups_x = (m_LightMeshletInfo->Triangle_Count + m_LightMeshletInfo->Meshlet_Size - 1) / m_LightMeshletInfo->Meshlet_Size;
			uint32_t num_workgroups_x = m_SceneLight->GetLightCount();
			uint32_t num_workgroups_y = 1;
			uint32_t num_workgroups_z = 1;

			std::vector<vk::DescriptorSet> arr{
				m_CamDescriptor->GetDescriptorSet(),
				m_LightDescriptor->GetDescriptorSet()
			};


			cmd[0].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
				m_MeshShaderLightPipeline->GetLayout(),
				uint32_t(0),
				arr, nullptr);

			cmd[0].drawMeshTasksEXT(num_workgroups_x, num_workgroups_y, num_workgroups_z);
		}

		cmd.End();
	}
}

void RenderLayer::LoadScene()
{
	// Load Meshes
	
	m_Scene->AddMesh("meshes/plane.obj", "Plane",
		Transformation{
			.position = {0, -1.5, 0}
	});
	m_Scene->AddMesh("meshes/wahoo.obj", "Wahoo",
		Transformation{
			.position = {0, -1.5, 0},
			.scale = {1, 1, 1}
	});
	//m_Scene->AddMesh("meshes/Astartes.obj", "Astartes",
	//	Transformation{
	//		.position = {0, -2, 0},
	//		.scale = {0.03f, 0.03f, 0.03f}
	//});
	//m_Scene->AddMesh("meshes/ignores/station.obj", "station",
	//	Transformation{
	//		.position = {0, -1.5, 0},
	//		.scale = {0.1f, 0.1f, 0.1f}
	//});
	//m_Scene->AddMesh("meshes/ignores/test_alpha.obj", "station",
	//	Transformation{
	//		.position = {0, -1.5, 0},
	//		.scale = {2.f, 1.f, 2.f}
	//});
	m_Scene->ComputeRenderData({
		.MeshletMaxPrimCount = 32,
		.MeshletMaxVertexCount = 255
	});

	// Load Lights
	float halfWidth = 1.0f;
	std::vector<glm::vec3> polygon_verts = {
		glm::vec3(-halfWidth, -halfWidth + 1.0f, -5.0f),
		glm::vec3(halfWidth, -halfWidth + 1.0f, -5.0f),
		glm::vec3(halfWidth, halfWidth + 1.0f, -5.0f),
		glm::vec3(-halfWidth, halfWidth + 1.0f, -5.0f),
	};
	std::array<glm::vec3, 4> bound_verts = {
		glm::vec3(-halfWidth, -halfWidth + 1.0f, -5.0f),
		glm::vec3(halfWidth, -halfWidth + 1.0f, -5.0f),
		glm::vec3(halfWidth, halfWidth + 1.0f, -5.0f),
		glm::vec3(-halfWidth, halfWidth + 1.0f, -5.0f),
	};
	glm::vec4 boundSphere;
	for (int i = 0;i < 4;++i) {
		boundSphere += glm::vec4(bound_verts[i], 0);
	}
	boundSphere /= 4.f;
	boundSphere.w = 20.f;
	MaterialInfo polygon_mat = MaterialInfo{
		.texPath = {
			"images/cyberpunk.jpg",
		}
	};
	//AreaLight polygon_light(
	//	AreaLightCreateInfo{
	//		.type = LIGHT_TYPE::POLYGON,
	//		.boundSphere = boundSphere,
	//		.transform = Transformation{
	//		//.rotation = glm::quat({glm::radians(90.f), 0, 0}),
	//		//.position = {0, 3, 0}
	//		},
	//		.boundPositions = bound_verts,
	//		.lightVertex = polygon_verts,
	//		.lightMaterial = polygon_mat
	//	}
	//);
	AreaLight polygon_light("meshes/lightQuad.obj");
	polygon_light.m_LightMaterial = polygon_mat;
	m_SceneLight->AddLight(polygon_light);

	std::vector<glm::vec3> bezier_verts = {
		glm::vec3(-1.5, -0.5f, 5.0f),
		glm::vec3(1.5f, -0.5f, 5.0f),
		glm::vec3(10.5f, -0.5f, 5.0f),
		glm::vec3(1.5f, 2.5f, 5.0f),
		glm::vec3(1.5f, 2.5f, 5.0f),
		glm::vec3(-1.5f, 3.5f, 5.0f),
		glm::vec3(-2.5f, 2.5f, 5.0f),
		glm::vec3(-1.5f, -0.5f, 5.0f)
	};
	std::array<glm::vec3, 4> bezier_bound_verts = {
		glm::vec3(-5.0f, -1.f, 5.0f),
		glm::vec3(4.0f, -1.f, 5.0f),
		glm::vec3(4.0f, 8.f, 5.0f),
		glm::vec3(-5.0f, 8.f, 5.0f),
	};
	glm::vec4 bezier_bound_sphere;
	for (int i = 0;i < 4;++i) {
		bezier_bound_sphere += glm::vec4(bezier_bound_verts[i],0);
	}
	bezier_bound_sphere /= 4.f;
	bezier_bound_sphere.w = 20.f;
	MaterialInfo bezier_mat = MaterialInfo{
		.texPath = {
			"images/test_image.jpg",
		}
	};
	m_SceneLight->AddLight(AreaLight{ 
		AreaLightCreateInfo{
			.type = LIGHT_TYPE::BEZIER,
			.boundSphere = bezier_bound_sphere,
			.boundPositions = bezier_bound_verts,
			.lightVertex = bezier_verts,
			.lightMaterial = bezier_mat
		}
	});
}

void RenderLayer::GenBuffers()
{
	// Create Camera buffer
	CameraUBO camera_ubo;
	camera_ubo.pos = glm::vec4(m_Camera.get()->GetTransform().position, 1);
	camera_ubo.viewProjMat = m_Camera->GetProjViewMatrix();
	camera_ubo.planes = m_Camera->GetPlanes();
	m_CamBuffer->CreateFromData(&camera_ubo, sizeof(CameraUBO), vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive);

	// Create Buffers from meshlets
	m_MeshletInfoBuffer->CreateFromData(m_Scene->GetMeshlets()->GetMeshletInfos().data(), sizeof(MeshletDescription) * m_Scene->GetMeshlets()->GetMeshletInfos().size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
	m_VertexIndicesBuffer->CreateFromData(m_Scene->GetMeshlets()->GetVertexIndices().data(), sizeof(uint32_t) * m_Scene->GetMeshlets()->GetVertexIndices().size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
	m_PrimitiveIndicesBuffer->CreateFromData(m_Scene->GetMeshlets()->GetPrimitiveIndices().data(), sizeof(uint8_t) * m_Scene->GetMeshlets()->GetPrimitiveIndices().size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
	m_VertexBuffer->CreateFromData(m_Scene->GetMeshlets()->GetVertices().data(), sizeof(Vertex) * m_Scene->GetMeshlets()->GetVertices().size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);

	m_ModelMatrixBuffer->CreateFromData(m_Scene->GetModelMatries().data(), m_Scene->GetModelMatries().size() * sizeof(ModelMatrix), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);

	// LightBuffers
	std::vector<LightInfo> lightBufferData = m_SceneLight->GetPackedLightInfo();
	m_LightBuffer->CreateFromData(lightBufferData.data(), lightBufferData.size() * sizeof(LightInfo), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
	auto idx = lightBufferData.size() * sizeof(LightInfo);
	int light_count = lightBufferData.size();
	m_LightCountBuffer->CreateFromData(&light_count, sizeof(int), vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive);

	// materials buffers
	glm::vec4 v(0.f);
	m_MaterialParamBuffer->CreateFromData(&v, sizeof(glm::vec4), vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive);
}

void RenderLayer::GenTextures()
{
	
	// Load dds image for LTC
	m_DDSTexture->CreateFromFile("images/ltc.dds", { .format = vk::Format::eR32G32B32A32Sfloat, .usage = vk::ImageUsageFlagBits::eSampled });
	m_DDSTexture->TransitionLayout(VK_ImageLayout{
		.layout = vk::ImageLayout::eShaderReadOnlyOptimal,
		.accessFlag = vk::AccessFlagBits::eShaderRead,
		.pipelineStage = vk::PipelineStageFlagBits::eFragmentShader,
	});
	m_DDSAmpFresnel->CreateFromFile("images/ltc_amp.dds", { .format = vk::Format::eR32G32Sfloat, .usage = vk::ImageUsageFlagBits::eSampled });
	m_DDSAmpFresnel->TransitionLayout(VK_ImageLayout{
	.layout = vk::ImageLayout::eShaderReadOnlyOptimal,
	.accessFlag = vk::AccessFlagBits::eShaderRead,
	.pipelineStage = vk::PipelineStageFlagBits::eFragmentShader,
	});
	
	// Scene Compress textures
	
	m_CompressedTexture->CreateFromData(m_Scene->GetAtlasTex2D()->GetData().data(),
		m_Scene->GetAtlasTex2D()->GetSize(),
		{
			.width = static_cast<uint32_t>(m_Scene->GetAtlasTex2D()->GetResolution().x),
			.height = static_cast<uint32_t>(m_Scene->GetAtlasTex2D()->GetResolution().y),
			.depth = 1,
		},
		{
			.format = vk::Format::eR8G8B8A8Unorm,
			.usage = vk::ImageUsageFlagBits::eSampled,
			.arrayLayer = 4
		}
	);

	m_CompressedTexture->TransitionLayout(VK_ImageLayout{
		.layout = vk::ImageLayout::eShaderReadOnlyOptimal,
		.accessFlag = vk::AccessFlagBits::eShaderRead,
		.pipelineStage = vk::PipelineStageFlagBits::eFragmentShader,
	});

	// light Blur textures
	AtlasTexture2D compressedLightTex = m_SceneLight->GetLightTexture();
	m_LightBlurTexture->CreateFromData(
		compressedLightTex.GetData().data(),
		compressedLightTex.GetSize(),
		{
			.width = static_cast<uint32_t>(compressedLightTex.GetResolution().x),
			.height = static_cast<uint32_t>(compressedLightTex.GetResolution().y),
			.depth = 1
		},
		{
			.format = vk::Format::eR8G8B8A8Unorm,
			.usage = vk::ImageUsageFlagBits::eSampled,
			.arrayLayer = m_SceneLight->GetBlurLayerCount()
		}
	);

	m_LightBlurTexture->TransitionLayout(
		VK_ImageLayout{
			.layout = vk::ImageLayout::eShaderReadOnlyOptimal,
			.accessFlag = vk::AccessFlagBits::eShaderRead,
			.pipelineStage = vk::PipelineStageFlagBits::eFragmentShader,
	});
}

void RenderLayer::CreateDescriptors()
{
	// Create Descriptors
	m_MaterialParamDescriptor->Create({
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eUniformBuffer,
			.stage = vk::ShaderStageFlagBits::eFragment,
			.bufferInfo = vk::DescriptorBufferInfo{
				.buffer = m_MaterialParamBuffer->GetBuffer(),
				.offset = 0,
				.range = m_MaterialParamBuffer->GetSize()
			}
		}
		});

	m_CamDescriptor->Create({
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eUniformBuffer,
			.stage = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment,
			.bufferInfo = vk::DescriptorBufferInfo{
				.buffer = m_CamBuffer->GetBuffer(),
				.offset = 0,
				.range = m_CamBuffer->GetSize()
			}
		}
		});

	m_LightDescriptor->Create({
			VK_DescriptorBinding{
				.type = vk::DescriptorType::eUniformBuffer,
				.stage = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = m_LightCountBuffer->GetBuffer(),
					.offset = 0,
					.range = m_LightCountBuffer->GetSize(),
				}
			},
			VK_DescriptorBinding{
				.type = vk::DescriptorType::eStorageBuffer,
				.stage = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = m_LightBuffer->GetBuffer(),
					.offset = 0,
					.range = m_LightBuffer->GetSize(),
				}
			},
			VK_DescriptorBinding{
				.type = vk::DescriptorType::eCombinedImageSampler,
				.stage = vk::ShaderStageFlagBits::eFragment,
				.imageInfo = vk::DescriptorImageInfo{
					.sampler = m_LightBlurTexture->GetSampler(),
					.imageView = m_LightBlurTexture->GetImageView(),
					.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
				}
			},
	});

	m_LTCMeshShaderInputDescriptor->Create({
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eStorageBuffer,
			.stage = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT,
			.bufferInfo = vk::DescriptorBufferInfo{
				.buffer = m_MeshletInfoBuffer->GetBuffer(),
				.offset = 0,
				.range = m_MeshletInfoBuffer->GetSize()
			}
		},
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eStorageBuffer,
			.stage = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT,
			.bufferInfo = vk::DescriptorBufferInfo{
				.buffer = m_ModelMatrixBuffer->GetBuffer(),
				.offset = 0,
				.range = m_ModelMatrixBuffer->GetSize()
			}
		},
		VK_DescriptorBinding{
				.type = vk::DescriptorType::eStorageBuffer,
				.stage = vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = m_VertexIndicesBuffer->GetBuffer(),
					.offset = 0,
					.range = m_VertexIndicesBuffer->GetSize()
				}
		},
		VK_DescriptorBinding{
				.type = vk::DescriptorType::eStorageBuffer,
				.stage = vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = m_PrimitiveIndicesBuffer->GetBuffer(),
					.offset = 0,
					.range = m_PrimitiveIndicesBuffer->GetSize()
				}
		},
		VK_DescriptorBinding{
				.type = vk::DescriptorType::eStorageBuffer,
				.stage = vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = m_VertexBuffer->GetBuffer(),
					.offset = 0,
					.range = m_VertexBuffer->GetSize()
				}
		},
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eCombinedImageSampler,
			.stage = vk::ShaderStageFlagBits::eFragment,
			.imageInfo = vk::DescriptorImageInfo{
				.sampler = m_DDSTexture->GetSampler(),
				.imageView = m_DDSTexture->GetImageView(),
				.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
			}
		},
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eCombinedImageSampler,
			.stage = vk::ShaderStageFlagBits::eFragment,
			.imageInfo = vk::DescriptorImageInfo{
			.sampler = m_DDSAmpFresnel->GetSampler(),
			.imageView = m_DDSAmpFresnel->GetImageView(),
			.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
			}
		},
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eCombinedImageSampler,
			.stage = vk::ShaderStageFlagBits::eFragment,
			.imageInfo = vk::DescriptorImageInfo{
				.sampler = m_CompressedTexture->GetSampler(),
				.imageView = m_CompressedTexture->GetImageView(),
				.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
			}
		},
		}
	);
}

void RenderLayer::CreateGraphicsPipeline()
{
	m_MeshShaderLightPipeline->CreateMeshPipeline(MeshPipelineCreateInfo
	{
		.descriptorSetsLayout = {
			m_CamDescriptor->GetDescriptorSetLayout(),
			m_LightDescriptor->GetDescriptorSetLayout(),
		},
		.taskShaderPath = "shaders/mesh_light.task.spv",
		.meshShaderPath = "shaders/mesh_light.mesh.spv",
		.fragShaderPath = "shaders/mesh_light.frag.spv"
	});

	m_MeshShaderLTCPipeline->CreateMeshPipeline(MeshPipelineCreateInfo
	{
		.descriptorSetsLayout = {
			m_CamDescriptor->GetDescriptorSetLayout(),
			m_LTCMeshShaderInputDescriptor->GetDescriptorSetLayout(),
			m_LightDescriptor->GetDescriptorSetLayout(),
			m_MaterialParamDescriptor->GetDescriptorSetLayout(),
		},
		.taskShaderPath = "shaders/mesh_ltc.task.spv",
		.meshShaderPath = "shaders/mesh_ltc.mesh.spv",
			.fragShaderPath = "shaders/mesh_ltc.frag.spv"
	});
}
