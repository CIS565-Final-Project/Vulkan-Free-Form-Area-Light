#pragma once

#include <vulkan/vulkan.hpp>

namespace VK_Renderer
{
	class VK_Device;
	class VK_CommandPool;

	

	class VK_CommandBuffer
	{
	public:
		struct AllocateInfo
		{
			uint32_t count{ 1 };
			vk::CommandBufferLevel level{ vk::CommandBufferLevel::ePrimary };
		};

		struct BeginInfo
		{
			vk::CommandBufferUsageFlags usage;
			vk::CommandBufferInheritanceInfo inheritInfo{};
		};

	public:
		VK_CommandBuffer(const VK_Device& device, 
						 VK_CommandPool const& commandPool, 
						 AllocateInfo const& allocateInfo);

		VK_CommandBuffer(VK_CommandBuffer && commandbuffer);

		inline void Reset(const uint32_t& idx = 0) const { vk_CommandBuffers[idx]->reset(); }
		inline void Begin(BeginInfo const& beginInfo, const uint32_t& idx = 0) const
		{
			vk_CommandBuffers[idx]->begin(vk::CommandBufferBeginInfo{
				.flags = beginInfo.usage,
				.pInheritanceInfo = &beginInfo.inheritInfo
			});
		}
		inline void End(const uint32_t& idx = 0) const { vk_CommandBuffers[idx]->end(); }

		inline size_t Size() const { return vk_CommandBuffers.size(); }

		const vk::CommandBuffer& operator[](const uint32_t& idx) const { return vk_CommandBuffers[idx].get(); }

	protected:
		VK_CommandPool const& m_CommandPool;
		std::vector<vk::UniqueCommandBuffer> vk_CommandBuffers;
	};
}