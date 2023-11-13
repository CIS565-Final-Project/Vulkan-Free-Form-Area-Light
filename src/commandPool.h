#pragma once

#include <vulkan/vulkan.hpp>

namespace VK_Renderer
{
	class VK_Device;

	class VK_CommandPool
	{
	public:
		VK_CommandPool(VK_Device& device, uint32_t queueFamilyIdx);
		~VK_CommandPool();

	public:
		vk::CommandPool vk_CommandPool;

	protected:
		VK_Device& m_Device;
		uint32_t m_QueueFamilyIdx;
	};
}