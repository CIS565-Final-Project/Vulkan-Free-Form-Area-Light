#include "commandbuffer.h"

#include "device.h"
#include "commandPool.h"

namespace VK_Renderer
{
	VK_CommandBuffer::VK_CommandBuffer(const VK_Device& device,
										VK_CommandPool const& commandPool,
										AllocateInfo const& allocateInfo)
		: m_CommandPool(commandPool)
	{
		vk_CommandBuffers = device.GetDevice().allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{
			.commandPool = commandPool.vk_CommandPool,
			.level = allocateInfo.level,
			.commandBufferCount = allocateInfo.count
		});
	}

	VK_CommandBuffer::VK_CommandBuffer(VK_CommandBuffer && commandbuffer)
		: m_CommandPool(commandbuffer.m_CommandPool)
	{
		vk_CommandBuffers.swap(commandbuffer.vk_CommandBuffers);
	}
}