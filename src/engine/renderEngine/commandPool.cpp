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

	uPtr<VK_CommandBuffer> VK_CommandPool::AllocateCommandBuffers(uint32_t const& count) const
	{
		return mkU<VK_CommandBuffer>(m_Device, vk_CommandPool, count);
	}
	void VK_CommandPool::FreeCommandBuffer(VK_CommandBuffer const & commandBuffer) const
	{
		m_Device.GetDevice().freeCommandBuffers(vk_CommandPool, commandBuffer.vk_CommandBuffers);
	}
}
