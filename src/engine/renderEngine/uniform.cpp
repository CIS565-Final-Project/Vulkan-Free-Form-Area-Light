#include "uniform.h"

#include "device.h"

namespace VK_Renderer
{
	VK_BufferUniform::VK_BufferUniform(VK_Device const & device)
		:VK_Uniform(device)
	{
	}
	
	void VK_Uniform::Free()
	{
		if (vk_DescriptorSetLayout)
			m_Device.GetDevice().destroyDescriptorSetLayout(vk_DescriptorSetLayout);
	}

	void VK_BufferUniform::Create(vk::ShaderStageFlags stageFlags,
									std::vector<vk::DeviceSize> const& uniformSizes,
									uint32_t const& count)
	{
		std::vector<vk::DescriptorSetLayoutBinding> set_layout_bindings;
		for (uint32_t binding = 0; binding < uniformSizes.size(); ++binding)
		{
			set_layout_bindings.push_back(vk::DescriptorSetLayoutBinding{
					.binding = binding,
					.descriptorType = vk::DescriptorType::eUniformBuffer,
					.descriptorCount = 1,
					.stageFlags = stageFlags
			});
		}

		vk_DescriptorSetLayout = m_Device.GetDevice().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{
			.bindingCount = static_cast<uint32_t>(set_layout_bindings.size()),
			.pBindings = set_layout_bindings.data()
		});

		std::vector<vk::DescriptorSetLayout> vk_DescriptorSetLayouts(count, vk_DescriptorSetLayout);

		vk_DescriptorSets = m_Device.GetDevice().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			.descriptorPool = m_Device.vk_DescriptorPool,
			.descriptorSetCount = static_cast<uint32_t>(vk_DescriptorSetLayouts.size()),
			.pSetLayouts = vk_DescriptorSetLayouts.data()
		});

		for (int i = 0; i < vk_DescriptorSets.size(); ++i)
		{
			for (uint32_t binding = 0; binding < uniformSizes.size(); ++binding)
			{
				m_MappedBuffers.emplace_back(m_Device);

				m_MappedBuffers.back().Create(uniformSizes[binding], vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive);

				vk::DescriptorBufferInfo buffer_info{
					.buffer = m_MappedBuffers.back().vk_Buffer,
					.offset = 0,
					.range = uniformSizes[binding]
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
	}
	void VK_BufferUniform::Free()
	{
		VK_Uniform::Free();
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
	
	VK_StorageBufferUniform::VK_StorageBufferUniform(VK_Device const& device)
		:VK_Uniform(device)
	{
	}

	void VK_StorageBufferUniform::Create(vk::ShaderStageFlags stageFlags,
											std::vector<vk::DeviceSize> const& uniformSizes,
											uint32_t const& count)
	{
		std::vector<vk::DescriptorSetLayoutBinding> set_layout_bindings;
		for (uint32_t binding = 0; binding < uniformSizes.size(); ++binding)
		{
			set_layout_bindings.push_back(vk::DescriptorSetLayoutBinding{
					.binding = binding,
				.descriptorType = vk::DescriptorType::eStorageBuffer,
				.descriptorCount = 1,
				.stageFlags = stageFlags
			});
		}

		vk_DescriptorSetLayout = m_Device.GetDevice().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{
			.bindingCount = static_cast<uint32_t>(set_layout_bindings.size()),
			.pBindings = set_layout_bindings.data()
		});

		std::vector<vk::DescriptorSetLayout> vk_DescriptorSetLayouts(count, vk_DescriptorSetLayout);

		vk_DescriptorSets = m_Device.GetDevice().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			.descriptorPool = m_Device.vk_DescriptorPool,
			.descriptorSetCount = static_cast<uint32_t>(vk_DescriptorSetLayouts.size()),
			.pSetLayouts = vk_DescriptorSetLayouts.data()
		});

		for (int i = 0; i < vk_DescriptorSets.size(); ++i)
		{
			for (uint32_t binding = 0; binding < uniformSizes.size(); ++binding)
			{
				m_Buffers.emplace_back(m_Device);
				m_Buffers.back().Create(uniformSizes[binding], vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);

				vk::DescriptorBufferInfo buffer_info{
					.buffer = m_Buffers.back().vk_Buffer,
					.offset = 0,
					.range = uniformSizes[binding]
				};

				m_Device.GetDevice().updateDescriptorSets(vk::WriteDescriptorSet{
					.dstSet = vk_DescriptorSets[i],
					.dstBinding = binding,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eStorageBuffer,
					.pBufferInfo = &buffer_info
				}, nullptr);
			}
		}
	}

	void VK_StorageBufferUniform::Free()
	{
		VK_Uniform::Free();
		for (auto const& buffer : m_Buffers)
		{
			buffer.Free();
		}
		m_Buffers.clear();
	}
}