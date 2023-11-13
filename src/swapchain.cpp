#include "swapchain.h"

#include "device.h"

namespace VK_Renderer
{
	VK_Swapchain::VK_Swapchain(vk::PhysicalDevice physicalDevice, 
								vk::SurfaceKHR surface, 
								const QueueFamilyIndices& queueFamilyIdx,
								const uint32_t& width, const uint32_t& height)
		: vk_PhysicalDevice(physicalDevice),
		  vk_Surface(surface),
		  m_QueueFamilyIndices(queueFamilyIdx)
	{
		m_SwapchainSupportDetails = {
				.cpabilities = vk_PhysicalDevice.getSurfaceCapabilitiesKHR(vk_Surface),
				.surfaceFormats = vk_PhysicalDevice.getSurfaceFormatsKHR(vk_Surface),
				.presentModes = vk_PhysicalDevice.getSurfacePresentModesKHR(vk_Surface)
		};

		vk::SurfaceFormatKHR surface_format = ChooseSurfaceFormat(m_SwapchainSupportDetails.surfaceFormats);
		vk_ImageFormat = surface_format.format;
		vk::PresentModeKHR present_mode = ChoosePresentMode(m_SwapchainSupportDetails.presentModes);
		vk_ImageExtent = m_SwapchainSupportDetails.cpabilities.currentExtent;

		if (vk_ImageExtent.height == std::numeric_limits<uint32_t>::max())
		{
			vk_ImageExtent.width = static_cast<uint32_t>(width);
			vk_ImageExtent.height = static_cast<uint32_t>(height);
		}

		uint32_t img_count = m_SwapchainSupportDetails.cpabilities.minImageCount + 1;

		if (m_SwapchainSupportDetails.cpabilities.maxImageCount > 0 &&
			m_SwapchainSupportDetails.cpabilities.maxImageCount < img_count)
		{
			img_count = m_SwapchainSupportDetails.cpabilities.maxImageCount;
		}
		vk::SwapchainCreateInfoKHR create_info{
			.surface = vk_Surface,
			.minImageCount = img_count,
			.imageFormat = surface_format.format,
			.imageColorSpace = surface_format.colorSpace,
			.imageExtent = vk_ImageExtent,
			.imageArrayLayers = 1,
			.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
			.preTransform = m_SwapchainSupportDetails.cpabilities.currentTransform,
			.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
			.presentMode = present_mode,
			.clipped = vk::True
		};

		uint32_t queue_family_indices[] = { m_QueueFamilyIndices.GraphicsIdx(), m_QueueFamilyIndices.PresentIdx() };

		if (m_QueueFamilyIndices.GraphicsIdx() != m_QueueFamilyIndices.PresentIdx())
		{
			create_info.imageSharingMode = vk::SharingMode::eConcurrent;
			create_info.queueFamilyIndexCount = 2;
			create_info.pQueueFamilyIndices = queue_family_indices;
		}
		else
		{
			create_info.imageSharingMode = vk::SharingMode::eExclusive;
			create_info.queueFamilyIndexCount = 0;
			create_info.pQueueFamilyIndices = nullptr;
		}

		if (VK_Device::GetVkDevice().createSwapchainKHR(&create_info, nullptr, &vk_Swapchain) != vk::Result::eSuccess)
		{
			throw std::runtime_error("failed to create swap chain!");
		}

		vk_SwapchainImages = VK_Device::GetVkDevice().getSwapchainImagesKHR(vk_Swapchain);

		CreateImageViews();
	}

	VK_Swapchain::~VK_Swapchain()
	{
		VK_Device::GetVkDevice().destroySwapchainKHR(vk_Swapchain);
		for (const auto& fb : vk_Framebuffers)
		{
			VK_Device::GetVkDevice().destroyFramebuffer(fb);
		}
		for (const auto& view : vk_SwapchainImageViews)
		{
			VK_Device::GetVkDevice().destroyImageView(view);
		}
	}

	void VK_Swapchain::CreateImageViews()
	{
		vk_SwapchainImageViews.resize(vk_SwapchainImages.size());
		VkImageViewCreateInfo create_info;
		for (size_t i = 0; i < vk_SwapchainImageViews.size(); ++i)
		{
			vk_SwapchainImageViews[i] = VK_Device::GetVkDevice().createImageView(vk::ImageViewCreateInfo{
				.image = vk_SwapchainImages[i],
				.viewType = vk::ImageViewType::e2D,
				.format = vk_ImageFormat,
				.components = {vk::ComponentSwizzle::eIdentity},
				.subresourceRange = {
					.aspectMask = vk::ImageAspectFlagBits::eColor,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			});
		}
	}

	void VK_Swapchain::CreateFramebuffers(vk::RenderPass renderPass)
	{
		vk_Framebuffers.resize(vk_SwapchainImageViews.size());

		for (size_t i = 0; i < vk_Framebuffers.size(); ++i)
		{
			vk_Framebuffers[i] = VK_Device::GetVkDevice().createFramebuffer(vk::FramebufferCreateInfo{
				.renderPass = renderPass,
				.attachmentCount = 1,
				.pAttachments = &vk_SwapchainImageViews[i],
				.width = vk_ImageExtent.width,
				.height = vk_ImageExtent.height,
				.layers = 1
			});
		}
	}

	vk::SurfaceFormatKHR VK_Swapchain::ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const
	{
		for (const auto& surface_format : availableFormats)
		{
			if (surface_format.format == vk::Format::eR8G8B8A8Srgb && 
				surface_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
			{
				return surface_format;
			}
		}

		return availableFormats[0];
	}

	vk::PresentModeKHR VK_Swapchain::ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availableModes) const
	{
		for (const auto& mode : availableModes)
		{
			if (mode == vk::PresentModeKHR::eMailbox)
			{
				return mode;
			}
		}

		return vk::PresentModeKHR::eFifo;
	}
}