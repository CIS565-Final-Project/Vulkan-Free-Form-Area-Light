#include "commandPool.h"

#include "device.h"
#include "commandBuffer.h"

namespace VK_Renderer
{
	VK_CommandPool::VK_CommandPool(VK_Device& device, uint32_t queueFamilyIdx)
		: m_Device(device),
		  m_QueueFamilyIdx(queueFamilyIdx)
	{
		vk_CommandPool = m_Device.GetDevice().createCommandPool(vk::CommandPoolCreateInfo{
			.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			.queueFamilyIndex = queueFamilyIdx
		});
	}

	VK_CommandPool::~VK_CommandPool()
	{
		if (vk_CommandPool)
		{
			m_Device.GetDevice().destroyCommandPool(vk_CommandPool);
		}
	}

	VK_CommandBuffer VK_CommandPool::AllocateCommandBuffers(VK_CommandBuffer::AllocateInfo const& allocateInfo) const
	{
		return { m_Device, *this, allocateInfo };
	}
}
