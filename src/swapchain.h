#pragma once

#include "common.h"
#include "vulkanUtils.h"
#include <vulkan/vulkan.hpp>

namespace VK_Renderer
{
	struct SwapchainSupportDetails
	{
		vk::SurfaceCapabilitiesKHR cpabilities;
		std::vector<vk::SurfaceFormatKHR> surfaceFormats;
		std::vector<vk::PresentModeKHR> presentModes;
	};

	class VK_Swapchain
	{
	public:
		VK_Swapchain(vk::PhysicalDevice physicalDevice, 
					 vk::SurfaceKHR surface,
					 const QueueFamilyIndices& queueFamilyIdx,
					 const uint32_t& width, const uint32_t& height);
		~VK_Swapchain();

		void CreateImageViews();
		void CreateFramebuffers(vk::RenderPass renderPass);

	protected:
		vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const;
		vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availableModes) const;

	protected:
		vk::PhysicalDevice vk_PhysicalDevice;
		vk::SurfaceKHR vk_Surface;

	public:
		vk::SwapchainKHR vk_Swapchain;
		vk::Format vk_ImageFormat;
		vk::Extent2D vk_ImageExtent;

		SwapchainSupportDetails m_SwapchainSupportDetails;

		std::vector<vk::Image> vk_SwapchainImages;
		std::vector<vk::ImageView> vk_SwapchainImageViews;
		std::vector<vk::Framebuffer> vk_Framebuffers;
		
		QueueFamilyIndices m_QueueFamilyIndices;
	};
}