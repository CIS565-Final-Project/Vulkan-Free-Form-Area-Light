#include "commandbuffer.h"

namespace VK_Renderer
{
	VK_CommandBuffer::VK_CommandBuffer(VkDevice device, VkCommandPool commandPool)
		: m_LogicalDevice(device), m_CommandPool(commandPool)
	{
		VkCommandBuffer command_buffer;
		VkCommandBufferAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.commandPool = commandPool;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(m_LogicalDevice, &alloc_info, &m_CommandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to Allocate Command Buffer!");
		}
	}

	VK_CommandBuffer::~VK_CommandBuffer()
	{
	}

	void VK_CommandBuffer::BeginRecordBuffer()
	{
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(m_CommandBuffer, &begin_info) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to Begin Record Command Buffer!");
		}
	}

	void VK_CommandBuffer::EndRecordBuffer()
	{
		if (vkEndCommandBuffer(m_CommandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to End Record Command Buffer!");
		}
	}
}