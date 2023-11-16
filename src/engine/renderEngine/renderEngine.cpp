#include "renderEngine.h"

#include "instance.h"
#include "device.h"
#include "swapchain.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace VK_Renderer
{
	VK_RenderEngine::VK_RenderEngine()
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

		m_Swapchain = mkU<VK_Swapchain>(*m_Device,
										SwapchainSupportDetails{
											.cpabilities = m_Instance->vk_PhysicalDevice.getSurfaceCapabilitiesKHR(surface),
											.surfaceFormats = m_Instance->vk_PhysicalDevice.getSurfaceFormatsKHR(surface),
											.presentModes = m_Instance->vk_PhysicalDevice.getSurfacePresentModesKHR(surface)
										},
										surface,
										m_Instance->m_QueueFamilyIndices,
										width, height);
	}

	void VK_RenderEngine::Reset()
	{
		m_Swapchain.reset();
		m_Device.reset();
		m_Instance.reset();
	}
}