#pragma once

#include "common.h"
#include <vulkan/vulkan.h>

namespace VK_Renderer
{
	class VK_CommandPool
	{
	public:
		VK_CommandPool(VkDevice device, uint32_t graphicsQueueFamilyIndex);
		~VK_CommandPool();

	public:
		VkCommandPool m_CommandPool;
	protected:
		VkDevice m_LogicalDevice;
		uint32_t m_GraphicsQueueFamilyIndex;
	};
}