#pragma once

#include "vulkanUtils.h"

#include <vulkan/vulkan.hpp>

namespace VK_Renderer
{
	class VK_CommandPool;

	struct DeviceProperties
	{
		vk::PhysicalDeviceProperties properties;
		vk::PhysicalDeviceMemoryProperties memoryProperties;
		vk::SampleCountFlagBits maxSampleCount{ vk::SampleCountFlagBits::e1 };
	};

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

		vk::UniqueDeviceMemory AllocateImageMemory(vk::Image image) const;

		void CreateDescriptiorPool(uint32_t const & descriptorCount, uint32_t const& maxSets);

		uint32_t GetMemoryTypeIndex(uint32_t typeBits, vk::MemoryPropertyFlags properties) const;

		vk::DescriptorPool vk_DescriptorPool;

	protected:
		vk::SampleCountFlagBits GetMaxSampleCount() const;

	protected:
		DeclareWithGetFunc(protected, vk::Device, vk, Device, const);
		DeclareWithGetFunc(protected, vk::PhysicalDevice, vk, PhysicalDevice, const);

		DeclareWithGetFunc(protected, vk::Queue, vk, GraphicsQueue, const);
		DeclareWithGetFunc(protected, vk::Queue, vk, PresentQueue, const);
		DeclareWithGetFunc(protected, vk::Queue, vk, ComputeQueue, const);
		DeclareWithGetFunc(protected, vk::Queue, vk, TransferQueue, const);

		DeclarePtrWithGetFunc(protected, uPtr, VK_CommandPool, vk, GraphicsCommandPool, const);
		DeclarePtrWithGetFunc(protected, uPtr, VK_CommandPool, vk, PresentCommandPool, const);
		DeclarePtrWithGetFunc(protected, uPtr, VK_CommandPool, vk, ComputeCommandPool, const);
		DeclarePtrWithGetFunc(protected, uPtr, VK_CommandPool, vk, TransferCommandPool, const);

		DeclareWithGetFunc(protected, DeviceProperties, m, DeviceProperties, const);
	};
}