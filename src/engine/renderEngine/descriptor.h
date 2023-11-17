#pragma once

#include <vulkan/vulkan.hpp>

#include "buffer.h"

namespace VK_Renderer
{
	class VK_Device;

	struct VK_DescriptorBinding
	{
		vk::DescriptorType type;
		vk::ShaderStageFlags stage;
		vk::DescriptorBufferInfo bufferInfo {};
		vk::DescriptorImageInfo imageInfo {};
	};

	class VK_Descriptor
	{
	public:
		VK_Descriptor(VK_Device const& device);
		~VK_Descriptor();

		void Create(std::vector<VK_DescriptorBinding> const& bindings);
		void Free();

	protected:
		VK_Device const& m_Device;

		DeclareWithGetFunc(protected, vk::DescriptorSet, vk, DescriptorSet, const);
		DeclareWithGetFunc(protected, vk::DescriptorSetLayout, vk, DescriptorSetLayout, const);
	};
}