#include "commandbuffer.h"

#include "device.h"

namespace VK_Renderer
{
	VK_CommandBuffer::VK_CommandBuffer(const VK_Device& device, vk::CommandPool commandPool, uint32_t count)
	{
		vk_CommandBuffers = device.NativeDevice().allocateCommandBuffers(vk::CommandBufferAllocateInfo{
			.commandPool = commandPool,
			.level = vk::CommandBufferLevel::ePrimary,
			.commandBufferCount = count
		});
	}
}