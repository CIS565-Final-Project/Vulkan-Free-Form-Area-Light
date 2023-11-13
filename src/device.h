#pragma once

#include "common.h"
#include "vulkanUtils.h"

#include <vulkan/vulkan.hpp>

namespace VK_Renderer
{
	// Custom feature handler
	class VK_Device
	{
	public:
		VK_Device(vk::PhysicalDevice physicalDevice, 
					const std::vector<const char*>& deviceExtensions, 
					vk::PhysicalDeviceFeatures2 physicalDeviceFeatures2,
					const QueueFamilyIndices& queueFamilyIdx,
					const uint32_t queue_count = 1);
		~VK_Device();

		inline vk::Device NativeDevice()		const { return vk_Device; }
		inline vk::Queue NativeGraphicsQueue()	const { return vk_GraphicsQueue; }
		inline vk::Queue NativePresentQueue()	const { return vk_PresentQueue; }
		inline vk::Queue NativeComputeQueue()	const { return vk_ComputeQueue; }
		inline vk::Queue NativeTransferQueue()	const { return vk_TransferQueue; }

	public:
		vk::Device vk_Device;

		vk::Queue vk_GraphicsQueue;
		vk::Queue vk_PresentQueue;
		vk::Queue vk_ComputeQueue;
		vk::Queue vk_TransferQueue;
	};
}