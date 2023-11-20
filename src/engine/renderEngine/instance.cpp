#include "instance.h"
#include <iostream>

#include <vulkan/vulkan.hpp>

#ifdef NDEBUG
const bool ENABLE_VALIDATION = true;
#else
const bool ENABLE_VALIDATION = true;
#endif

namespace VK_Renderer
{
	const std::vector<const char*> ValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
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

	VK_Instance::VK_Instance(std::vector<char const*>const& instanceExtensions, const std::string& app_name)
	{		
		vk::ApplicationInfo app_info{
			.pApplicationName = app_name.c_str(),
			.pEngineName = "No Engine",
			.engineVersion = vk::makeVersion(1, 0, 0),
			.apiVersion = vk::ApiVersion13
		};

		vk::InstanceCreateInfo create_info = {
			.pApplicationInfo = &app_info
		};

		std::vector<char const*> exts = instanceExtensions;

		// create a separate debug utils messenger specifically for 
		// vkCreateInstance and vkDestroyInstance
		vk::DebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info = CreateDebugMessengerCreateInfo();

		if (ENABLE_VALIDATION)
		{
			if (!CheckValidationLayerSupport()) throw std::runtime_error("Validation Layers not Support!");

			create_info.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
			create_info.ppEnabledLayerNames = ValidationLayers.data();
			create_info.pNext = &debug_utils_messenger_create_info;
			exts.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			exts.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		} 

		create_info.enabledExtensionCount = exts.size();
		create_info.ppEnabledExtensionNames = exts.data();
		
		vk_Instance = vk::createInstance(create_info);

		// Load Instance extension functions after creatation
		VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_Instance);

		if (ENABLE_VALIDATION && SetupDebugMessenger() != vk::Result::eSuccess) 
			throw std::runtime_error("Failed to create DebugUtilsMessenger!");
	}
	
	VK_Instance::~VK_Instance()
	{
		if (vk_Surface) vk_Instance.destroySurfaceKHR(vk_Surface);
		if (vk_DebugUtilsMessenger) vk_Instance.destroyDebugUtilsMessengerEXT(vk_DebugUtilsMessenger);
		vk_Instance.destroy();
	}

	void VK_Instance::PickPhysicalDeivce()
	{
		std::vector<vk::PhysicalDevice> phy_devices = vk_Instance.enumeratePhysicalDevices();

#ifndef NDEBUG
		printf("%d available devices\n", phy_devices.size());
#endif 

		if (phy_devices.size() == 0) throw std::runtime_error("Not available devices for Vulkan!");

		for (const auto& phy_device : phy_devices)
		{
			if (IsPhysicalDeviceSuitable(phy_device))
			{
				vk_PhysicalDevice = phy_device;
				break;
			}
		}
		if (vk_PhysicalDevice == nullptr)
			throw std::runtime_error("Not suitable physical device found!");
	}

	bool VK_Instance::CheckDeviceExtensionSupport(VkPhysicalDevice device)
	{
		return true;
		//uint32_t extension_count;
		//vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
		//
		//std::vector<VkExtensionProperties> available_extensions(extension_count);
		//vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());
		//
		//std::set<std::string> required_extensions(DeviceExtensions.begin(), DeviceExtensions.end());
		//
		//for (const auto& extension : available_extensions)
		//{
		//	required_extensions.erase(extension.extensionName);
		//}
		//
		//return required_extensions.empty();
	}

	vk::DebugUtilsMessengerCreateInfoEXT VK_Instance::CreateDebugMessengerCreateInfo()
	{
		return {
			.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
								vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
								vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
			.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | 
							vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | 
							vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
			.pfnUserCallback = DebugCallBack,
		};
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

	vk::Result VK_Instance::SetupDebugMessenger()
	{
		vk::DebugUtilsMessengerCreateInfoEXT create_info = CreateDebugMessengerCreateInfo();
		
		// Since using VULKAN_HPP_DISPATCH_LOADER_DYNAMIC, don't need to manually load extension functions
		return vk_Instance.createDebugUtilsMessengerEXT(&create_info, nullptr, &vk_DebugUtilsMessenger);
	}

	bool VK_Instance::IsPhysicalDeviceSuitable(vk::PhysicalDevice device)
	{
		//VkPhysicalDeviceProperties phy_dev_properties;
		//VkPhysicalDeviceFeatures phy_dev_features;
		//
		//vkGetPhysicalDeviceProperties(device, &phy_dev_properties);
		//vkGetPhysicalDeviceFeatures(device, &phy_dev_features);
		//
		m_QueueFamilyIndices = FindQueueFamilies(device);
		//bool extension_support = CheckDeviceExtensionSupport(device);
		//bool swapchain_adequate = false;
		//if (extension_support)
		//{
		//	m_SwapchainSupportDetails = QuerySwapchainSupport(device);
		//	swapchain_adequate = !m_SwapchainSupportDetails.surfaceFormats.empty() &&
		//							!m_SwapchainSupportDetails.presentModes.empty();
		//}
		//
		//
		//return m_QueueFamilyIndices.Valid() && swapchain_adequate && extension_support &&
		//		phy_dev_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		//		phy_dev_features.geometryShader;
		return true;
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

	QueueFamilyIndices VK_Instance::FindQueueFamilies(vk::PhysicalDevice physicalDevice)
	{
		QueueFamilyIndices indices;

		std::vector<vk::QueueFamilyProperties> properties = physicalDevice.getQueueFamilyProperties();

		for (int i = 0; i < properties.size(); ++i)
		{
			const vk::QueueFamilyProperties& queue_family_property = properties[i];

			vk::Bool32 present_support = false;

			present_support = physicalDevice.getSurfaceSupportKHR(i, vk_Surface);

			if (present_support)
			{
				indices.presentFamily = i;
			}

			if (!indices.graphicsFamily.has_value() && 
				queue_family_property.queueFlags & vk::QueueFlagBits::eGraphics)
			{
				indices.graphicsFamily = i;
			}
			if (!indices.computeFamily.has_value() &&
				queue_family_property.queueFlags & vk::QueueFlagBits::eCompute)
			{
				indices.computeFamily = i;
			}
			if (!indices.memTransferFamily.has_value() &&
				queue_family_property.queueFlags & vk::QueueFlagBits::eTransfer)
			{
				indices.memTransferFamily = i;
			}

			if (indices.presentFamily.has_value() &&
				indices.graphicsFamily.has_value() &&
				indices.computeFamily.has_value() &&
				indices.memTransferFamily.has_value())
			{
				break;
			}
		}

		return indices;
	}
}