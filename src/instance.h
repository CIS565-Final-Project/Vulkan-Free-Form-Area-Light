#pragma once

#include <vulkan/vulkan.h>
#include <SDL_vulkan.h>

#include "common.h"

namespace VK_Renderer
{
	struct QueueFamilyIndices 
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool Valid() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
		uint32_t GraphicsValue() const { return graphicsFamily.value(); }
		uint32_t PresentValue() const { return presentFamily.value(); }
	};

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class VK_Instance
	{
	public:
		VK_Instance(SDL_Window* window, const std::string& app_name);
		~VK_Instance();

		void PickPhysicalDeivce();
		void CreateLogicDevice();
		void CreateSwapchain(const int& w, const int& h);
		void CreateImageViews();
		void CreateFrameBuffers(VkRenderPass renderPass);

	protected:
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		bool CheckValidationLayerSupport();
		VkResult SetupDebugMessenger();
		bool IsPhysicalDeviceSuitable(VkPhysicalDevice device);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
		SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);
		VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
		VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& availableMode) const;
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	public:
		VkInstance m_Instance;
		VkSurfaceKHR m_Surface;

		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_LogicalDevice;
		VkSwapchainKHR m_Swapchain;
		VkFormat m_SwapchainImageFormat;
		VkExtent2D m_SwapchainExtent;

		VkDebugUtilsMessengerEXT m_DebugUtilsMessenger;
		VkPhysicalDeviceMemoryProperties m_DeviceMemoryProperties;
		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;


		QueueFamilyIndices m_QueueFamilyIndices;
		SwapchainSupportDetails m_SwapchainSupportDetails;

		std::vector<const char*> m_Extensions;

		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;
		std::vector<VkFramebuffer> m_SwapchainFramebuffers;

	private:
		static VK_Instance* s_Instance;
	public:
		static void CheckAvailableExtensions();
		static VK_Instance* Global_Get_VK_Instance() { return s_Instance; }
	};
}
