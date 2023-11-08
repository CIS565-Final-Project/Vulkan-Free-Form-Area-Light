#pragma once

#include "common.h"
#include <vulkan/vulkan.h>

namespace VK_Renderer
{
	class VK_CommandBuffer
	{
	public:
		VK_CommandBuffer(VkDevice device, VkCommandPool commandPool);
		~VK_CommandBuffer();

		void BeginRecordBuffer();
		void EndRecordBuffer();


	public:
		VkCommandBuffer m_CommandBuffer;

	protected:
		VkDevice m_LogicalDevice;
		VkCommandPool m_CommandPool;
	};
}