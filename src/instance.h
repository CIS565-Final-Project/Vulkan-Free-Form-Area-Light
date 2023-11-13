#pragma once

#include <vulkan/vulkan.hpp>
#include <SDL_vulkan.h>

#include "common.h"
#include "vulkanUtils.h"

namespace VK_Renderer
{
	class VK_Instance
	{
	public:
		VK_Instance(SDL_Window* window, const std::string& app_name);
		~VK_Instance();

		void PickPhysicalDeivce();
		uint32_t GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const;

	protected:
		vk::DebugUtilsMessengerCreateInfoEXT CreateDebugMessengerCreateInfo();
		bool CheckValidationLayerSupport();
		VkResult SetupDebugMessenger();
		bool IsPhysicalDeviceSuitable(vk::PhysicalDevice device);
		QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice physicalDevice);

		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	public:
		vk::Instance vk_Instance;
		VkInstance m_Instance;
		VkSurfaceKHR m_Surface;

		VkPhysicalDevice m_PhysicalDevice;

		VkDebugUtilsMessengerEXT m_DebugUtilsMessenger;
		VkPhysicalDeviceMemoryProperties m_DeviceMemoryProperties;

		QueueFamilyIndices m_QueueFamilyIndices;

		std::vector<const char*> m_Extensions;
	private:
		static VK_Instance* s_Instance;
	public:
		static void CheckAvailableExtensions();

		static VK_Instance* Global_Get_VK_Instance() { return s_Instance; }
	};
}
