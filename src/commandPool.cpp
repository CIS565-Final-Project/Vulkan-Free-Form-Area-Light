#include "commandPool.h"

namespace VK_Renderer
{
	VK_CommandPool::VK_CommandPool(VkDevice device, uint32_t graphicsQueueFamilyIndex)
		: m_LogicalDevice(device),
		  m_GraphicsQueueFamilyIndex(graphicsQueueFamilyIndex),
		  m_CommandPool(VK_NULL_HANDLE)
	{
		VkCommandPoolCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		create_info.queueFamilyIndex = m_GraphicsQueueFamilyIndex;

		if (vkCreateCommandPool(device, &create_info, nullptr, &m_CommandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to Create Command Pool!");
		}
	}

	VK_CommandPool::~VK_CommandPool()
	{
		if (m_CommandPool)
		{
			vkDestroyCommandPool(m_LogicalDevice, m_CommandPool, nullptr);
		}
	}
}
