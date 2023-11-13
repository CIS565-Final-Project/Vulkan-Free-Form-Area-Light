#include "device.h"

namespace VK_Renderer
{
	VK_Device::VK_Device(vk::PhysicalDevice physicalDevice,
							const std::vector<const char*>& deviceExtensions,
							vk::PhysicalDeviceFeatures2 physicalDeviceFeatures2,
							const QueueFamilyIndices& queueFamilyIdx,
							const uint32_t queue_count)
	{
		std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
		std::set<uint32_t> unique_queues = 
		{ 
			queueFamilyIdx.GraphicsIdx(),
			queueFamilyIdx.PresentIdx(),
			queueFamilyIdx.ComputeIdx(),
			queueFamilyIdx.MemTransferIdx(),
		};

		float queue_priority = 1.f;

		for (uint32_t queue_family : unique_queues)
		{
			queue_create_infos.push_back(vk::DeviceQueueCreateInfo{
				.queueFamilyIndex = queue_family,
				.queueCount = queue_count,
				.pQueuePriorities = &queue_priority
			});
		}

		vk::DeviceCreateInfo create_info{
			.pNext = &physicalDeviceFeatures2,
			.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
			.pQueueCreateInfos = queue_create_infos.data(),
			.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
			.ppEnabledExtensionNames = deviceExtensions.data()
		};

		// Create Logical Device
		vk_Device = physicalDevice.createDevice(create_info);

		// Load Instance extension functions after creatation
		VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_Device);

		// Get queue handler
		vk_Device.getQueue(queueFamilyIdx.GraphicsIdx(), 0, &vk_GraphicsQueue);
		vk_Device.getQueue(queueFamilyIdx.PresentIdx(), 0, &vk_PresentQueue);
		vk_Device.getQueue(queueFamilyIdx.ComputeIdx(), 0, &vk_ComputeQueue);
		vk_Device.getQueue(queueFamilyIdx.MemTransferIdx(), 0, &vk_TransferQueue);

	}

	VK_Device::~VK_Device()
	{
		vk_Device.destroy();
	}
}