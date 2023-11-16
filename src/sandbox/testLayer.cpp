#include "testLayer.h"

#include <iostream>

#include <SDL.h>

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

	pipeline->CreatePipeline(shader_stages, input, descripotrSetLayouts);

	vk_device.destroyShaderModule(task_module);
	vk_device.destroyShaderModule(mesh_module);
	vk_device.destroyShaderModule(frag_module);
}

RenderLayer::RenderLayer(std::string const& name)
	: Layer(name), 
	  image_index(0)
{
}

void RenderLayer::OnAttach()
{
	m_Camera = mkU<PerspectiveCamera>();
	m_Camera->m_Transform = Transformation{
		.position = {0, 0, -10}
	};
	m_Camera->resolution = {680, 680};

	m_Device = Application::GetInstance()->GetRenderEngine()->GetDevice();
	m_Swapchain = Application::GetInstance()->GetRenderEngine()->GetSwapchain();

	m_CommandBuffer = m_Device->GetGraphicsCommandPool()->AllocateCommandBuffers();
	m_Device->CreateDescriptiorPool(1, 10);

	Mesh mesh;
	//mesh.LoadMeshFromFile("meshes/stanford_bunny.obj");
	mesh.LoadMeshFromFile("meshes/sphere.obj");
	//mesh.LoadMeshFromFile("meshes/cube.obj");

	// Create Uniforms
	m_CameraUniform = mkU<VK_BufferUniform>(*m_Device);
	m_MeshletUniform = mkU<VK_BufferUniform>(*m_Device);
	m_MeshletInfo = mkU<MeshletInfo>(32, static_cast<uint32_t>(mesh.m_Triangles.size()));
	
	m_CameraUniform->Create(vk::ShaderStageFlagBits::eMeshEXT, { sizeof(CameraUBO) }, 3);
	m_MeshletUniform->Create(vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT, { sizeof(MeshletInfo) }, 1);
	m_MeshletUniform->m_MappedBuffers[0].Update(m_MeshletInfo.get(), 0, sizeof(MeshletInfo));

	m_MeshShaderInputUniform = mkU<VK_StorageBufferUniform>(*m_Device);
	std::vector<glm::ivec4> triangles;

	for (Triangle const& tri : mesh.m_Triangles)
	{
		triangles.emplace_back(tri.pId, tri.materialId);
	}

	m_MeshShaderInputUniform->Create(vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT,
		{
			sizeof(glm::ivec4) * triangles.size(),
			sizeof(Float) * mesh.m_Positions.size(),
		},
		1);

	m_MeshShaderInputUniform->m_Buffers[0].Update(triangles.data(), 0, sizeof(glm::ivec4) * triangles.size());
	m_MeshShaderInputUniform->m_Buffers[1].Update(mesh.m_Positions.data(), 0, sizeof(Float) * mesh.m_Positions.size());

	// Create Pipeline
	m_MeshShaderPipeline = mkU<VK_GraphicsPipeline>(*m_Device,
		m_Swapchain->vk_ImageExtent,
		m_Swapchain->vk_ImageFormat);

	std::vector<vk::DescriptorSetLayout> descriptor_set_layouts{
		m_CameraUniform->vk_DescriptorSetLayout,
		m_MeshShaderInputUniform->vk_DescriptorSetLayout,
		m_MeshletUniform->vk_DescriptorSetLayout
	};
	CreateMeshPipeline(m_Device->GetDevice(), m_MeshShaderPipeline.get(), descriptor_set_layouts);
	m_Swapchain->CreateFramebuffers(m_MeshShaderPipeline->vk_RenderPass);

	image_available_semaphore = m_Device->GetDevice().createSemaphore(vk::SemaphoreCreateInfo{});
	render_finished_semaphore = m_Device->GetDevice().createSemaphore(vk::SemaphoreCreateInfo{});
	fence = m_Device->GetDevice().createFence(vk::FenceCreateInfo{
		.flags = vk::FenceCreateFlagBits::eSignaled
	});
}

void RenderLayer::OnDetech()
{
	vk::Result result = m_Device->GetDevice().waitForFences(fence, vk::True, UINT64_MAX);
	if (result != vk::Result::eSuccess)
	{
		std::cout << "wait for Fence failed!" << std::endl;;
	}
	m_Device->GetDevice().resetFences(fence);

	m_CameraUniform->Free();
	m_MeshletUniform->Free();
	m_MeshShaderInputUniform->Free();

	m_Device->GetDevice().destroyFence(fence);
	m_Device->GetDevice().destroySemaphore(image_available_semaphore);
	m_Device->GetDevice().destroySemaphore(render_finished_semaphore);

	m_MeshShaderPipeline.reset();

	m_Device->GetGraphicsCommandPool()->FreeCommandBuffer(*m_CommandBuffer);
}

void RenderLayer::OnUpdate(double const& deltaTime)
{
	static vk::ClearValue clear_color{};
	clear_color.setColor({ {{0.0f, 0.0f, 0.0f, 1.0f}} });

	m_Device->GetDevice().waitForFences(fence, vk::True, UINT64_MAX);
	m_Device->GetDevice().resetFences(fence);

	vk::ResultValue result = m_Device->GetDevice().acquireNextImageKHR(m_Swapchain->vk_Swapchain, UINT64_MAX, image_available_semaphore, nullptr);
	if (result.result != vk::Result::eSuccess)
	{
		throw std::runtime_error("Fail to acquire next image KHR");
	}
	image_index = result.value;
	m_CommandBuffer->Reset();

	// Record Command Buffer
	{
		VK_CommandBuffer& command_buffers = *m_CommandBuffer;
		command_buffers.Begin(vk::CommandBufferUsageFlagBits::eRenderPassContinue);

		command_buffers[0].beginRenderPass(vk::RenderPassBeginInfo{
			.renderPass = m_MeshShaderPipeline->vk_RenderPass,
			.framebuffer = m_Swapchain->vk_Framebuffers[image_index],
			.renderArea = vk::Rect2D{
				.offset = {0, 0},
				.extent = m_Swapchain->vk_ImageExtent
			},
			.clearValueCount = 1,
			.pClearValues = &clear_color,
			}, vk::SubpassContents::eInline);

		command_buffers[0].bindPipeline(vk::PipelineBindPoint::eGraphics, m_MeshShaderPipeline->vk_Pipeline);

		// Viewport and scissors
		command_buffers[0].setViewport(0, vk::Viewport{
			.x = 0.f,
			.y = 0.f,
			.width = static_cast<float>(m_Swapchain->vk_ImageExtent.width),
			.height = static_cast<float>(m_Swapchain->vk_ImageExtent.height),
			.minDepth = 0.f,
			.maxDepth = 1.f
			});

		command_buffers[0].setScissor(0, vk::Rect2D{
			.offset = { 0, 0 },
			.extent = m_Swapchain->vk_ImageExtent
			});

		// Draw call
		uint32_t num_workgroups_x = (m_MeshletInfo->Triangle_Count + m_MeshletInfo->Meshlet_Size - 1) / m_MeshletInfo->Meshlet_Size;
		uint32_t num_workgroups_y = 1;
		uint32_t num_workgroups_z = 1;
		m_Camera->RecomputeProjView();
		glm::mat4 view_proj_mat = m_Camera->GetProjViewMatrix();
		m_CameraUniform->m_MappedBuffers[image_index].Update(&view_proj_mat, 0, sizeof(glm::mat4));

		glm::vec4 v0 = view_proj_mat * glm::vec4(1., -1., 0, 1.f);
		glm::vec4 v1 = view_proj_mat * glm::vec4(1., 1., 0, 1.f);
		glm::vec4 v2 = view_proj_mat * glm::vec4(-1., -1., 0, 1.f);

		vk::ArrayProxy<vk::DescriptorSet> arr{
			m_CameraUniform->vk_DescriptorSets[image_index],
			m_MeshShaderInputUniform->vk_DescriptorSets[0],
			m_MeshletUniform->vk_DescriptorSets[0]
		};

		command_buffers[0].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			m_MeshShaderPipeline->vk_PipelineLayout,
			uint32_t(0),
			arr, nullptr);

		command_buffers[0].drawMeshTasksEXT(num_workgroups_x, num_workgroups_y, num_workgroups_z);

		command_buffers[0].endRenderPass();
		command_buffers.End();
	}
}

void RenderLayer::OnRender(double const& deltaTime)
{
	// Submit Command Buffer
	VK_CommandBuffer& command_buffers = *m_CommandBuffer;
	std::array<vk::Semaphore, 1> wait_semaphore{ image_available_semaphore };
	std::array<vk::Semaphore, 1> signal_semaphores{ render_finished_semaphore };

	std::array<vk::PipelineStageFlags, 1> wait_stages{ vk::PipelineStageFlagBits::eColorAttachmentOutput };

	m_Device->GetGraphicsQueue().submit(vk::SubmitInfo{
		.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphore.size()),
		.pWaitSemaphores = wait_semaphore.data(),
		.pWaitDstStageMask = wait_stages.data(),
		.commandBufferCount = 1,
		.pCommandBuffers = &command_buffers[0],
		.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size()),
		.pSignalSemaphores = signal_semaphores.data()
		}, fence);

	std::array<vk::SwapchainKHR, 1> swapchains{ m_Swapchain->vk_Swapchain };
	m_Device->GetPresentQueue().presentKHR(vk::PresentInfoKHR{
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signal_semaphores.data(),
		.swapchainCount = static_cast<uint32_t>(swapchains.size()),
		.pSwapchains = swapchains.data(),
		.pImageIndices = &image_index
		});
}

void RenderLayer::OnImGui(double const& deltaTime)
{
}

void RenderLayer::OnEvent(SDL_Event const& e)
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
		}
		if (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE))
		{
			glm::vec2 offset = 0.01f * glm::vec2(mouse_cur - mouse_pre);
			m_Camera->m_Transform.RotateAround(glm::vec3(0.f), { 0.1f * offset.y, -offset.x, 0 });
			m_Camera->RecomputeProjView();
		}
		mouse_pre = mouse_cur;
	}
}
