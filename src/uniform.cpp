#include "uniform.h"

#include "device.h"

namespace VK_Renderer
{
	VK_BufferUniform::VK_BufferUniform(VK_Device const & device)
		:VK_Uniform(device)
	{
	}

	void VK_BufferUniform::Create(uint32_t const& binding, 
									vk::ShaderStageFlags stageFlags,
									uint32_t const& count,
									vk::DeviceSize uniformSize)
	{
		vk_DescriptorSetLayoutBinding = {
			.binding = binding,
			.descriptorType = vk::DescriptorType::eUniformBuffer,
			.descriptorCount = 1,
			.stageFlags = stageFlags
		};

		vk_DescriptorSetLayout = m_Device.GetDevice().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{
			.bindingCount = 1,
			.pBindings = &vk_DescriptorSetLayoutBinding
		});

		std::vector<vk::DescriptorSetLayout> vk_DescriptorSetLayouts(count, vk_DescriptorSetLayout);

		vk_DescriptorSets = m_Device.GetDevice().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			.descriptorPool = m_Device.vk_DescriptorPool,
			.descriptorSetCount = static_cast<uint32_t>(vk_DescriptorSetLayouts.size()),
			.pSetLayouts = vk_DescriptorSetLayouts.data()
		});

		for (int i = 0; i < vk_DescriptorSets.size(); ++i)
		{
			m_MappedBuffers.emplace_back(m_Device);

			m_MappedBuffers.back().Create(uniformSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive, vk::MemoryPropertyFlags{});
		
			vk::DescriptorBufferInfo buffer_info{
				.buffer = m_MappedBuffers.back().vk_Buffer,
				.offset = 0,
				.range = uniformSize
			};

			m_Device.GetDevice().updateDescriptorSets(vk::WriteDescriptorSet{
				.dstSet = vk_DescriptorSets[i],
				.dstBinding = binding,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eUniformBuffer,
				.pBufferInfo = &buffer_info
			}, nullptr);
		}
	}
	void VK_BufferUniform::Free()
	{
		m_Device.GetDevice().destroyDescriptorSetLayout(vk_DescriptorSetLayout);
		for (auto const& buffer : m_MappedBuffers)
		{
			buffer.Free();
		}
		m_MappedBuffers.clear();
	}
	VK_Uniform::VK_Uniform(VK_Device const& device)
		: m_Device(device)
	{
	}
}