#include "swapchain.h"

#include "device.h"

namespace VK_Renderer
{
	VK_Swapchain::VK_Swapchain(const VK_Device& device,
								const SwapchainSupportDetails& details,
								vk::SurfaceKHR surface, 
								const QueueFamilyIndices& queueFamilyIdx,
								const uint32_t& width, const uint32_t& height)
		: m_Device(device),
		  vk_Surface(surface),
		  m_QueueFamilyIndices(queueFamilyIdx)
	{
		vk::SurfaceFormatKHR surface_format = ChooseSurfaceFormat(details.surfaceFormats);
		vk_ImageFormat = surface_format.format;
		vk::PresentModeKHR present_mode = ChoosePresentMode(details.presentModes);
		vk_ImageExtent = details.cpabilities.currentExtent;

		if (vk_ImageExtent.height == std::numeric_limits<uint32_t>::max())
		{
			vk_ImageExtent.width = static_cast<uint32_t>(width);
			vk_ImageExtent.height = static_cast<uint32_t>(height);
		}

		uint32_t img_count = details.cpabilities.minImageCount + 1;

		if (details.cpabilities.maxImageCount > 0 &&
			details.cpabilities.maxImageCount < img_count)
		{
			img_count = details.cpabilities.maxImageCount;
		}
		vk::SwapchainCreateInfoKHR create_info{
			.surface = vk_Surface,
			.minImageCount = img_count,
			.imageFormat = surface_format.format,
			.imageColorSpace = surface_format.colorSpace,
			.imageExtent = vk_ImageExtent,
			.imageArrayLayers = 1,
			.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
			.preTransform = details.cpabilities.currentTransform,
			.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
			.presentMode = present_mode,
			.clipped = vk::True,
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

		if (m_Device.GetDevice().createSwapchainKHR(&create_info, nullptr, &vk_Swapchain) != vk::Result::eSuccess)
		{
			throw std::runtime_error("failed to create swap chain!");
		}

		vk_SwapchainImages = m_Device.GetDevice().getSwapchainImagesKHR(vk_Swapchain);

		CreateImageViews();
	}

	VK_Swapchain::~VK_Swapchain()
	{
		for (const auto& fb : vk_Framebuffers)
		{
			m_Device.GetDevice().destroyFramebuffer(fb);
		}
		for (const auto& view : vk_SwapchainImageViews)
		{
			m_Device.GetDevice().destroyImageView(view);
		}

		m_Device.GetDevice().destroySwapchainKHR(vk_Swapchain);
	}

	void VK_Swapchain::CreateImageViews()
	{
		vk_SwapchainImageViews.resize(vk_SwapchainImages.size());
		VkImageViewCreateInfo create_info;
		for (size_t i = 0; i < vk_SwapchainImageViews.size(); ++i)
		{
			vk_SwapchainImageViews[i] = m_Device.GetDevice().createImageView(vk::ImageViewCreateInfo{
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

	void VK_Swapchain::CreateFramebuffers(vk::RenderPass renderPass, 
										std::vector<vk::ImageView> attachments)
	{
		vk_Framebuffers.resize(vk_SwapchainImageViews.size());

		attachments.insert(attachments.begin(), vk::ImageView());

		for (size_t i = 0; i < vk_Framebuffers.size(); ++i)
		{
			*attachments.begin() = vk_SwapchainImageViews[i];

			vk_Framebuffers[i] = m_Device.GetDevice().createFramebuffer(vk::FramebufferCreateInfo{
				.renderPass = renderPass,
				.attachmentCount = static_cast<uint32_t>(attachments.size()),
				.pAttachments = attachments.data(),
				.width = vk_ImageExtent.width,
				.height = vk_ImageExtent.height,
				.layers = 1,
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