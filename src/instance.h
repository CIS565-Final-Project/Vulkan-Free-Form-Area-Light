#pragma once

#include <vulkan/vulkan.hpp>
#include <SDL_vulkan.h>

#include "vulkanUtils.h"

namespace VK_Renderer
{
	class VK_Instance
	{
	public:
		VK_Instance(SDL_Window* window, const std::string& app_name);
		~VK_Instance();

		void PickPhysicalDeivce();
		uint32_t GetMemoryTypeIndex(uint32_t typeBits, vk::MemoryPropertyFlagBits properties) const;

	protected:
		vk::DebugUtilsMessengerCreateInfoEXT CreateDebugMessengerCreateInfo();
		bool CheckValidationLayerSupport();
		vk::Result SetupDebugMessenger();
		bool IsPhysicalDeviceSuitable(vk::PhysicalDevice device);
		QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice physicalDevice);

		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	public:
		vk::Instance vk_Instance;
		vk::SurfaceKHR vk_Surface;

		vk::PhysicalDevice vk_PhysicalDevice;

		vk::DebugUtilsMessengerEXT vk_DebugUtilsMessenger;
		vk::PhysicalDeviceMemoryProperties vk_DeviceMemoryProperties;

		QueueFamilyIndices m_QueueFamilyIndices;

		std::vector<const char*> m_Extensions;

	public:
		static void CheckAvailableExtensions();
	};
}
