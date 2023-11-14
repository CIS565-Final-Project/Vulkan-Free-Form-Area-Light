#pragma once

#include <vulkan/vulkan.hpp>

namespace VK_Renderer
{
	class VK_Device;

	class VK_CommandBuffer
	{
	public:
		VK_CommandBuffer(const VK_Device& device, vk::CommandPool commandPool, uint32_t count = 1);

		inline void Reset(const uint32_t& idx = 0) const { vk_CommandBuffers[idx].reset(); }
		inline void Begin(vk::CommandBufferUsageFlags flags, const uint32_t& idx = 0) const 
		{
			vk_CommandBuffers[idx].begin(vk::CommandBufferBeginInfo{
				.flags = flags
			}); 
		}
		inline void End(const uint32_t& idx = 0) const { vk_CommandBuffers[idx].end(); }

		const vk::CommandBuffer& operator[](const uint32_t& idx) const { return vk_CommandBuffers[idx]; }

	public:
		std::vector<vk::CommandBuffer> vk_CommandBuffers;
	};
}