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

	protected:
		VK_Device const& m_Device;

	public:
		vk::DescriptorSetLayoutBinding vk_DescriptorSetLayoutBinding;
		vk::DescriptorSetLayout vk_DescriptorSetLayout;
	};

	class VK_BufferUniform : public VK_Uniform
	{
	public:
		VK_BufferUniform(VK_Device const & device);

		void Create(uint32_t const& binding,
					vk::ShaderStageFlags stageFlags,
					uint32_t const& count,
					vk::DeviceSize uniformSize);

		void Free();

		std::vector<vk::DescriptorSet> vk_DescriptorSets;
		std::vector<VK_MappedBuffer> m_MappedBuffers;
	};
}