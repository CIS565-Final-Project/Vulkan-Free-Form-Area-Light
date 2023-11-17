#pragma once

#include <vulkan/vulkan.hpp>

#include "buffer.h"

namespace VK_Renderer
{
	class VK_Device;

	class VK_Uniform
	{
	public:
		VK_Uniform(VK_Device const& device);

		virtual void Create(vk::ShaderStageFlags stageFlags,
							std::vector<vk::DeviceSize> const& uniformSizes,
							uint32_t const& count) = 0;

		virtual void Free();

	protected:
		VK_Device const& m_Device;

	public:
		vk::DescriptorSetLayout vk_DescriptorSetLayout;
	};

	class VK_BufferUniform : public VK_Uniform
	{
	public:
		VK_BufferUniform(VK_Device const & device);

		void Create(vk::ShaderStageFlags stageFlags,
					std::vector<vk::DeviceSize> const& uniformSizes,
					uint32_t const& count);

		void Free();

		std::vector<vk::DescriptorSet> vk_DescriptorSets;
		std::vector<VK_StagingBuffer> m_MappedBuffers;
	};

	class VK_StorageBufferUniform : public VK_Uniform
	{
	public:
		VK_StorageBufferUniform(VK_Device const& device);

		void Create(vk::ShaderStageFlags stageFlags,
					std::vector<vk::DeviceSize> const& uniformSizes,
					uint32_t const& count);

		void Free();

		std::vector<vk::DescriptorSet> vk_DescriptorSets;
		std::vector<VK_DeviceBuffer> m_Buffers;
	};
}