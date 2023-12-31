#pragma once

#include "vulkanUtils.h"
#include <vulkan/vulkan.hpp>

namespace VK_Renderer
{
	class VK_Device;

	class VK_Swapchain
	{
	public:
		VK_Swapchain(const VK_Device& device, 
					 const SwapchainSupportDetails& details,
					 vk::SurfaceKHR surface,
					 const QueueFamilyIndices& queueFamilyIdx,
					 const uint32_t& width, const uint32_t& height);
		~VK_Swapchain();

		void CreateImageViews();
		void CreateFramebuffers(vk::RenderPass renderPass, 
								std::vector<vk::ImageView> attachments);

		bool AcquireNextImage();
		bool Present(std::vector<vk::Semaphore> const& waitSemaphores);

	protected:
		vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const;
		vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availableModes) const;

	protected:
		const VK_Device& m_Device;
		vk::SurfaceKHR vk_Surface;

		DeclareWithGetFunc(protected, uint32_t, m, ImageIdx, const);
		DeclareWithGetFunc(protected, vk::Semaphore, vk, ImageAviableSemaphore, const);

		vk::UniqueSemaphore vk_UniqueImageAviableSemaphore;

	public:
		vk::SwapchainKHR vk_Swapchain;
		vk::Format vk_ImageFormat;
		vk::Extent2D vk_ImageExtent;

		std::vector<vk::Image> vk_SwapchainImages;
		std::vector<vk::ImageView> vk_SwapchainImageViews;
		std::vector<vk::Framebuffer> vk_Framebuffers;
		
		QueueFamilyIndices m_QueueFamilyIndices;
	};
}