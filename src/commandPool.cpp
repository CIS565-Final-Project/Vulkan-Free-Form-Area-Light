#include "commandPool.h"

#include "device.h"

namespace VK_Renderer
{
	VK_CommandPool::VK_CommandPool(VK_Device& device, uint32_t queueFamilyIdx)
		: m_Device(device),
		  m_QueueFamilyIdx(queueFamilyIdx)
	{
		vk_CommandPool = m_Device.NativeDevice().createCommandPool(vk::CommandPoolCreateInfo{
			.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			.queueFamilyIndex = queueFamilyIdx
		});
	}

	VK_CommandPool::~VK_CommandPool()
	{
		if (vk_CommandPool)
		{
			m_Device.NativeDevice().destroyCommandPool(vk_CommandPool);
		}
	}
}
