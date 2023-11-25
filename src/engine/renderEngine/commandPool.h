#pragma once

#include <vulkan/vulkan.hpp>
#include "commandBuffer.h"

namespace VK_Renderer
{
	class VK_CommandPool
	{
	public:
		VK_CommandPool(VK_Device& device, uint32_t queueFamilyIdx);
		~VK_CommandPool();

		VK_CommandBuffer AllocateCommandBuffers(VK_CommandBuffer::AllocateInfo const& allocateInfo = {}) const;

	public:
		vk::CommandPool vk_CommandPool;

	protected:
		VK_Device& m_Device;
		uint32_t m_QueueFamilyIdx;
	};
}