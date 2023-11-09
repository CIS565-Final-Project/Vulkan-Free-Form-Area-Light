#include "instance.h"
#include <iostream>

#include <vulkan/vulkan.hpp>

#ifdef NDEBUG
const bool ENABLE_VALIDATION = false;
#else
const bool ENABLE_VALIDATION = true;
#endif

namespace VK_Renderer
{
	const std::vector<const char*> ValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
	
	const std::vector<const char*> DeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallBack(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	)
	{
		std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	VK_Instance* VK_Instance::s_Instance = nullptr;

	VK_Instance::VK_Instance(SDL_Window* window, const std::string& app_name)
		: m_PhysicalDevice(VK_NULL_HANDLE),
		  m_LogicalDevice(VK_NULL_HANDLE),
		  m_Swapchain(VK_NULL_HANDLE)
	{
		assert(!s_Instance);
		s_Instance = this;

		VkApplicationInfo app_info = {};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pEngineName = "Vulkan Renderer Engine";
		app_info.apiVersion = VK_API_VERSION_1_3;
		app_info.pApplicationName = app_name.c_str();
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

		VkInstanceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;

		unsigned int sdl_ext_count = 0;
		SDL_Vulkan_GetInstanceExtensions(window, &sdl_ext_count, NULL);

		m_Extensions.resize(sdl_ext_count);
		SDL_Vulkan_GetInstanceExtensions(window, &sdl_ext_count, m_Extensions.data());

		// create a separate debug utils messenger specifically for 
		// vkCreateInstance and vkDestroyInstance
		VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info;
		PopulateDebugMessengerCreateInfo(debug_utils_messenger_create_info);

		if (ENABLE_VALIDATION)
		{
			if (!CheckValidationLayerSupport()) throw std::runtime_error("Validation Layers not Support!");

			create_info.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
			create_info.ppEnabledLayerNames = ValidationLayers.data();
			create_info.pNext = &debug_utils_messenger_create_info;
			m_Extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		} 

		create_info.enabledExtensionCount = m_Extensions.size();
		create_info.ppEnabledExtensionNames = m_Extensions.data();

		if (vkCreateInstance(&create_info, nullptr, &m_Instance) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create instance");
		}

		if (ENABLE_VALIDATION && SetupDebugMessenger() != VK_SUCCESS) 
			throw std::runtime_error("Failed to create DebugUtilsMessenger!");
	}
	
	VK_Instance::~VK_Instance()
	{
		if (ENABLE_VALIDATION)
		{
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr)
			{
				func(m_Instance, m_DebugUtilsMessenger, nullptr);
			}
		}
		
		for (auto framebuffer : m_SwapchainFramebuffers) {
			vkDestroyFramebuffer(m_LogicalDevice, framebuffer, nullptr);
		}

		for (auto imageView : m_SwapchainImageViews) {
			vkDestroyImageView(m_LogicalDevice, imageView, nullptr);
		}

		vkDestroySwapchainKHR(m_LogicalDevice, m_Swapchain, nullptr);
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyDevice(m_LogicalDevice, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
		s_Instance = nullptr;
	}

	void VK_Instance::PickPhysicalDeivce()
	{
		uint32_t dev_count = 0;
		std::vector<VkPhysicalDevice> phy_devices(Max_Physical_Device_Count);
		vkEnumeratePhysicalDevices(m_Instance, &dev_count, nullptr);

#ifndef NDEBUG
		printf("%d available devices\n", dev_count);	
#endif 

		if (dev_count == 0) throw std::runtime_error("Not available devices for Vulkan!");
		vkEnumeratePhysicalDevices(m_Instance, &dev_count, phy_devices.data());

		for (const auto& phy_device : phy_devices)
		{
			if (IsPhysicalDeviceSuitable(phy_device))
			{
				m_PhysicalDevice = phy_device;
				break;
			}
		}
		if (m_PhysicalDevice == VK_NULL_HANDLE)
			throw std::runtime_error("Not suitable physical device found!");

		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_DeviceMemoryProperties);
	}

	void VK_Instance::CreateLogicDevice()
	{
		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		std::set<uint32_t> unique_queues = { m_QueueFamilyIndices.GraphicsValue(), m_QueueFamilyIndices.PresentValue() };

		float queue_priority = 1.f;

		for (uint32_t queue_family : unique_queues)
		{
			VkDeviceQueueCreateInfo queue_create_info = {};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = m_QueueFamilyIndices.GraphicsValue();
			queue_create_info.queueCount = 1;
			queue_create_info.pQueuePriorities = &queue_priority;

			queue_create_infos.push_back(queue_create_info);
		}

		VkPhysicalDeviceFeatures device_features = {};

		VkDeviceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		create_info.pQueueCreateInfos = queue_create_infos.data();
		create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
		create_info.pEnabledFeatures = &device_features;

		create_info.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
		create_info.ppEnabledExtensionNames = DeviceExtensions.data();

		if (vkCreateDevice(m_PhysicalDevice, &create_info, nullptr, &m_LogicalDevice) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to Create Logical Device!");
		}

		vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.GraphicsValue(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.PresentValue(), 0, &m_PresentQueue);
	}

	void VK_Instance::CreateSwapchain(const int& w, const int& h)
	{
		m_SwapchainSupportDetails = QuerySwapchainSupport(m_PhysicalDevice);

		VkSurfaceFormatKHR surface_format = ChooseSurfaceFormat(m_SwapchainSupportDetails.surfaceFormats);
		m_SwapchainImageFormat = surface_format.format;
		VkPresentModeKHR present_mode = ChoosePresentMode(m_SwapchainSupportDetails.presentModes);
		m_SwapchainExtent = ChooseSwapExtent(m_SwapchainSupportDetails.surfaceCapabilities);

		if (m_SwapchainExtent.height == std::numeric_limits<uint32_t>::max())
		{
			m_SwapchainExtent.width  = static_cast<uint32_t>(w);
			m_SwapchainExtent.height = static_cast<uint32_t>(h);
		}

		uint32_t img_count = m_SwapchainSupportDetails.surfaceCapabilities.minImageCount + 1;

		if (m_SwapchainSupportDetails.surfaceCapabilities.maxImageCount > 0 &&
			m_SwapchainSupportDetails.surfaceCapabilities.maxImageCount < img_count)
		{
			img_count = m_SwapchainSupportDetails.surfaceCapabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		create_info.surface = m_Surface;
		create_info.minImageCount = img_count;
		create_info.imageFormat = surface_format.format;
		create_info.imageColorSpace = surface_format.colorSpace;
		create_info.imageExtent = m_SwapchainExtent;
		create_info.imageArrayLayers = 1;
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t queue_family_indices[] = { m_QueueFamilyIndices.GraphicsValue(), m_QueueFamilyIndices.PresentValue() };

		if (m_QueueFamilyIndices.GraphicsValue() != m_QueueFamilyIndices.PresentValue())
		{
			create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			create_info.queueFamilyIndexCount = 2;
			create_info.pQueueFamilyIndices = queue_family_indices;
		}
		else
		{
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.queueFamilyIndexCount = 0;
			create_info.pQueueFamilyIndices = nullptr;
		}

		create_info.preTransform = m_SwapchainSupportDetails.surfaceCapabilities.currentTransform;
		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		create_info.presentMode = present_mode;
		create_info.clipped = VK_TRUE;

		create_info.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(m_LogicalDevice, &create_info, nullptr, &m_Swapchain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swap chain!");
		}

		img_count = 0;
		vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain, &img_count, nullptr);
		m_SwapchainImages.resize(img_count);
		vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain, &img_count, m_SwapchainImages.data());
	}

	void VK_Instance::CreateImageViews()
	{
		m_SwapchainImageViews.resize(m_SwapchainImages.size());
		VkImageViewCreateInfo create_info;
		for (size_t i = 0; i < m_SwapchainImageViews.size(); ++i)
		{
			create_info = {};
			create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

			create_info.image = m_SwapchainImages[i];

			create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			create_info.format = m_SwapchainImageFormat;

			create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			create_info.subresourceRange.baseMipLevel = 0;
			create_info.subresourceRange.levelCount = 1;
			create_info.subresourceRange.baseArrayLayer = 0;
			create_info.subresourceRange.layerCount = 1;

			if (vkCreateImageView(m_LogicalDevice, &create_info, nullptr, &m_SwapchainImageViews[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create Image View!");
			}
		}
	}

	void VK_Instance::CreateFrameBuffers(VkRenderPass renderPass)
	{
		m_SwapchainFramebuffers.resize(m_SwapchainImageViews.size());

		for (size_t i = 0; i < m_SwapchainFramebuffers.size(); ++i)
		{
			std::array<VkImageView, 1> attachment{m_SwapchainImageViews[i]};

			VkFramebufferCreateInfo create_info = {};

			create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			create_info.renderPass = renderPass;
			create_info.attachmentCount = 1;
			create_info.pAttachments = attachment.data();
			create_info.width = m_SwapchainExtent.width;
			create_info.height = m_SwapchainExtent.height;
			create_info.layers = 1;

			if (vkCreateFramebuffer(m_LogicalDevice, &create_info, nullptr, &m_SwapchainFramebuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create Frambuffer!");
			}
		}
	}

	QueueFamilyIndices VK_Instance::FindQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queue_families_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, nullptr);

		std::vector<VkQueueFamilyProperties> queue_families(queue_families_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, queue_families.data());

		for (int i = 0; i < queue_families_count; ++i)
		{
			const auto& queue_family = queue_families[i];

			VkBool32 present_support = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &present_support);
			
			if (present_support)
			{
				indices.presentFamily = i;
			}

			if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
				break;
			}
		}

		return indices;
	}

	bool VK_Instance::CheckDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extension_count;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

		std::set<std::string> required_extensions(DeviceExtensions.begin(), DeviceExtensions.end());

		for (const auto& extension : available_extensions)
		{
			required_extensions.erase(extension.extensionName);
		}

		return required_extensions.empty();
	}

	SwapchainSupportDetails VK_Instance::QuerySwapchainSupport(VkPhysicalDevice device)
	{
		SwapchainSupportDetails swapchain_support_details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &swapchain_support_details.surfaceCapabilities);

		uint32_t format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &format_count, nullptr);
		if (format_count > 0)
		{
			swapchain_support_details.surfaceFormats.resize(format_count);

			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &format_count,
				swapchain_support_details.surfaceFormats.data());
		}

		uint32_t present_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &present_count, nullptr);
		if (present_count > 0)
		{
			swapchain_support_details.presentModes.resize(present_count);

			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &present_count,
				swapchain_support_details.presentModes.data());
		}

		return swapchain_support_details;
	}

	VkSurfaceFormatKHR VK_Instance::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const
	{
		for (const auto& surface_format : availableFormats)
		{
			if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB && surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return surface_format;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR VK_Instance::ChoosePresentMode(const std::vector<VkPresentModeKHR>& availableMode) const
	{
		for (const VkPresentModeKHR& mode : availableMode)
		{
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return mode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D VK_Instance::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		return capabilities.currentExtent;
	}

	void VK_Instance::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		createInfo.pfnUserCallback = DebugCallBack;
		createInfo.pUserData = nullptr;
	}

	bool VK_Instance::CheckValidationLayerSupport()
	{
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

		if (layer_count > 0)
		{
			std::vector<VkLayerProperties> layer_properties(layer_count);
			vkEnumerateInstanceLayerProperties(&layer_count, layer_properties.data());
			for (const char* layer : ValidationLayers)
			{
				bool support = false;
				for (const auto& property : layer_properties)
				{
					if (strcmp(layer, property.layerName) == 0)
					{
						support = true;
						break;
					}
				}
				if (!support) return false;
			}
			return true;
		}
		return false;
	}

	VkResult VK_Instance::SetupDebugMessenger()
	{
		VkDebugUtilsMessengerCreateInfoEXT create_info = {};
		PopulateDebugMessengerCreateInfo(create_info);

		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			return func(m_Instance, &create_info, nullptr, &m_DebugUtilsMessenger);
		}
		else
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	bool VK_Instance::IsPhysicalDeviceSuitable(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties phy_dev_properties;
		VkPhysicalDeviceFeatures phy_dev_features;

		vkGetPhysicalDeviceProperties(device, &phy_dev_properties);
		vkGetPhysicalDeviceFeatures(device, &phy_dev_features);

		m_QueueFamilyIndices = FindQueueFamilies(device);
		bool extension_support = CheckDeviceExtensionSupport(device);
		bool swapchain_adequate = false;
		if (extension_support)
		{
			m_SwapchainSupportDetails = QuerySwapchainSupport(device);
			swapchain_adequate = !m_SwapchainSupportDetails.surfaceFormats.empty() &&
									!m_SwapchainSupportDetails.presentModes.empty();
		}
		

		return m_QueueFamilyIndices.Valid() && swapchain_adequate && extension_support &&
				phy_dev_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
				phy_dev_features.geometryShader;
	}

	void VK_Instance::CheckAvailableExtensions()
	{
		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

		if (extension_count > 0)
		{
			std::vector<VkExtensionProperties> extensions(extension_count);

			vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

			printf("%d available instance extensions:\n", extension_count);
			for (const auto& extension : extensions)
			{
				printf("%s\n", extension.extensionName);
			}
		}
	}

	uint32_t VK_Instance::GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const {
		// Iterate over all memory types available for the device used in this example
		for (uint32_t i = 0; i < m_DeviceMemoryProperties.memoryTypeCount; i++) {
			if ((typeBits & 1) == 1) {
				if ((m_DeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
					return i;
				}
			}
			typeBits >>= 1;
		}
		throw std::runtime_error("Could not find a suitable memory type!");
	}
}