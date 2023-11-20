#include "renderEngine.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace VK_Renderer
{
	VK_RenderEngine::VK_RenderEngine()
		: vk_ClearColor({ { {0.f, 0.f, 0.f, 1.f} } })
	{
		// Load necessary functions before any function call
		VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

#ifndef NDEBUG
		VK_Instance::CheckAvailableExtensions();
#endif
	}
	void VK_RenderEngine::Init(std::vector<char const*>const& instanceExtensions,
								CreateSurfaceFN createSurfaceFunc,
								std::vector<char const*>const& deviceExtensions,
								vk::PhysicalDeviceFeatures2 phyDevFeature2,
								int const& width,
								int const& height)
	{
		m_Instance = mkU<VK_Instance>(instanceExtensions, "Vulkan Render Engine");

		// Create surface for rendering
		VkSurfaceKHR surface;
		if (createSurfaceFunc(m_Instance->vk_Instance, &surface) != true)
		{
			throw std::runtime_error("SDL_Vulkan_CreateSurface Failed!");
		}
		m_Instance->vk_Surface = surface;

		// Select a suitable Physical Device from avaiable physical devices (graphics cards)
		m_Instance->PickPhysicalDeivce();
		
		m_Instance->vk_PhysicalDevice.getFeatures2(&phyDevFeature2);

		// Create Vulkan Logical devices with desired extensions
		m_Device = mkU<VK_Device>(m_Instance->vk_PhysicalDevice,
								  deviceExtensions,
								  phyDevFeature2,
								  m_Instance->m_QueueFamilyIndices);
		m_Device->CreateDescriptiorPool(1, 10);
		
		// Create Swapchain
		m_Swapchain = mkU<VK_Swapchain>(*m_Device,
										SwapchainSupportDetails{
											.cpabilities = m_Instance->vk_PhysicalDevice.getSurfaceCapabilitiesKHR(surface),
											.surfaceFormats = m_Instance->vk_PhysicalDevice.getSurfaceFormatsKHR(surface),
											.presentModes = m_Instance->vk_PhysicalDevice.getSurfacePresentModesKHR(surface)
										},
										surface,
										m_Instance->m_QueueFamilyIndices,
										width, height);

		// Create RenderPass
		m_RenderPass = mkU<VK_RenderPass>(*m_Device);
		m_RenderPass->Create(m_Swapchain->vk_ImageFormat);

		// Create Textures
		m_DepthTexture = mkU<VK_Texture2D>(*m_Device);
		m_ColorTexture = mkU<VK_Texture2D>(*m_Device);

		m_DepthTexture->Create(vk::Extent3D{ m_Swapchain->vk_ImageExtent.width, m_Swapchain->vk_ImageExtent.height, 1 },
								TextureCreateInfo{ .format = vk::Format::eD32Sfloat,
												   .aspectMask = vk::ImageAspectFlagBits::eDepth,
												   .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
												   .sampleCount = m_Device->GetDeviceProperties().maxSampleCount });

		m_DepthTexture->TransitionLayout({ .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
											.accessFlag = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
											.pipelineStage = vk::PipelineStageFlagBits::eEarlyFragmentTests
										 });

		m_ColorTexture->Create(vk::Extent3D{ m_Swapchain->vk_ImageExtent.width, m_Swapchain->vk_ImageExtent.height, 1 },
								TextureCreateInfo{ .format = m_Swapchain->vk_ImageFormat,
												   .aspectMask = vk::ImageAspectFlagBits::eColor,
												   .usage = vk::ImageUsageFlagBits::eColorAttachment,
												   .sampleCount = m_Device->GetDeviceProperties().maxSampleCount });
		
		// Create FrameBuffer
		m_Swapchain->CreateFramebuffers(m_RenderPass->GetRenderPass(), { m_DepthTexture->GetImageView(), m_ColorTexture->GetImageView() });

		// Create Command Buffers
		m_CommandBuffer = mkU<VK_CommandBuffer>(
			m_Device->GetGraphicsCommandPool()->AllocateCommandBuffers(
				{ .count = static_cast<uint32_t>(m_Swapchain->vk_Framebuffers.size()) }
		));

		// Create RenderFinish Semaphores
		vk_UniqueRenderFinishedSemaphore = m_Device->GetDevice().createSemaphoreUnique(vk::SemaphoreCreateInfo{});
		m_RenderFinishSemaphores.push_back(vk_UniqueRenderFinishedSemaphore.get());

		m_SecondaryCommands.resize(m_CommandBuffer->Size());

		// Create Fences
		for (int i = 0; i < m_Swapchain->vk_Framebuffers.size(); ++i)
		{
			m_Fences.push_back(m_Device->GetDevice().createFenceUnique(vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}));
		}
	}

	void VK_RenderEngine::Reset()
	{
		WaitIdle();
		m_Fences.clear();
		m_CommandBuffer.reset();

		vk_UniqueRenderFinishedSemaphore.reset();

		m_RenderPass.reset();
		m_DepthTexture.reset();
		m_ColorTexture.reset();

		m_Swapchain.reset();
		m_Device.reset();
		m_Instance.reset();
	}

	void VK_RenderEngine::WaitIdle() const
	{
		m_Device->GetTransferQueue().waitIdle();
		m_Device->GetComputeQueue().waitIdle();
		m_Device->GetGraphicsQueue().waitIdle();
		m_Device->GetPresentQueue().waitIdle();
	}

	void VK_RenderEngine::WaitForFence() {
		uint32_t const& image_idx = m_Swapchain->GetImageIdx();
		m_Device->GetDevice().waitForFences(m_Fences[image_idx].get(), vk::True, std::numeric_limits<uint32_t>().max());
		m_Device->GetDevice().resetFences(m_Fences[image_idx].get());
	}

	void VK_RenderEngine::RecordCommandBuffer()
	{
		static std::array<vk::ClearValue, 3> clear_color{};
		clear_color[0].setColor(vk_ClearColor);
		clear_color[1].setDepthStencil({ .depth = 1.f, .stencil = 0 });
		clear_color[2].setColor(vk_ClearColor);

		uint32_t const& image_idx = m_Swapchain->GetImageIdx();

		//m_Device->GetGraphicsQueue().waitIdle();
		m_CommandBuffer->Reset(image_idx);
		m_CommandBuffer->Begin({ .usage = vk::CommandBufferUsageFlagBits::eOneTimeSubmit }, image_idx);

		(*m_CommandBuffer)[image_idx].beginRenderPass(vk::RenderPassBeginInfo{
			.renderPass = m_RenderPass->GetRenderPass(),
			.framebuffer = m_Swapchain->vk_Framebuffers[image_idx],
			.renderArea = vk::Rect2D{
				.offset = {0, 0},
				.extent = m_Swapchain->vk_ImageExtent
			},
			.clearValueCount = clear_color.size(),
			.pClearValues = clear_color.data(),
			}, vk::SubpassContents::eSecondaryCommandBuffers);

		if (m_SecondaryCommands[image_idx].size() > 0)
		{
			(*m_CommandBuffer)[image_idx].executeCommands(static_cast<uint32_t>(m_SecondaryCommands[image_idx].size()), m_SecondaryCommands[image_idx].data());
		}

		(*m_CommandBuffer)[image_idx].endRenderPass();
		(*m_CommandBuffer).End(image_idx);

	}

	void VK_RenderEngine::BeforeRender()
	{
		m_Swapchain->AcquireNextImage();
		WaitForFence();
	}

	void VK_RenderEngine::Render()
	{
		VK_CommandBuffer& command_buffers = *m_CommandBuffer;
		std::vector<vk::Semaphore> wait_semaphore{ m_Swapchain->GetImageAviableSemaphore()};
		std::array<vk::Semaphore, 1> signal_semaphores{ vk_UniqueRenderFinishedSemaphore.get()};

		std::array<vk::PipelineStageFlags, 1> wait_stages{ vk::PipelineStageFlagBits::eColorAttachmentOutput };

		std::vector<vk::CommandBuffer> cmds = m_PrimaryCommands;
		cmds.push_back(command_buffers[m_Swapchain->GetImageIdx()]);

		m_Device->GetGraphicsQueue().submit(vk::SubmitInfo{
			.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphore.size()),
			.pWaitSemaphores = wait_semaphore.data(),
			.pWaitDstStageMask = wait_stages.data(),
			.commandBufferCount = static_cast<uint32_t>(cmds.size()),
			.pCommandBuffers = cmds.data(),
			.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size()),
			.pSignalSemaphores = signal_semaphores.data(),
		}, m_Fences[m_Swapchain->GetImageIdx()].get());

		m_Swapchain->Present(m_RenderFinishSemaphores);
	}
}