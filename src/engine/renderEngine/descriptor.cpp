#include "descriptor.h"

#include "device.h"

namespace VK_Renderer
{
	VK_Descriptor::VK_Descriptor(VK_Device const& device)
		:m_Device(device)
	{
	}

	VK_Descriptor::~VK_Descriptor()
	{
		Free();
	}

	void VK_Descriptor::Create(std::vector<VK_DescriptorBinding> const& bindings)
	{
		std::vector<vk::DescriptorSetLayoutBinding> set_layout_bindings;
		for (uint32_t i = 0; i < bindings.size(); ++i)
		{
			set_layout_bindings.push_back(vk::DescriptorSetLayoutBinding{
				.binding = i,
				.descriptorType = bindings[i].type,
				.descriptorCount = 1,
				.stageFlags = bindings[i].stage
			});
		}

		vk_DescriptorSetLayout = m_Device.GetDevice().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{
			.bindingCount = static_cast<uint32_t>(set_layout_bindings.size()),
			.pBindings = set_layout_bindings.data()
		});

		vk_DescriptorSet = m_Device.GetDevice().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			.descriptorPool = m_Device.vk_DescriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &vk_DescriptorSetLayout
		})[0];

		for (uint32_t i = 0; i < bindings.size(); ++i)
		{
			m_Device.GetDevice().updateDescriptorSets(vk::WriteDescriptorSet{
				.dstSet = vk_DescriptorSet,
				.dstBinding = i,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = bindings[i].type,
				.pImageInfo = &bindings[i].imageInfo,
				.pBufferInfo = &bindings[i].bufferInfo,
			}, nullptr);
		}
	}
	void VK_Descriptor::Free()
	{
		if(vk_DescriptorSetLayout) 
			m_Device.GetDevice().destroyDescriptorSetLayout(vk_DescriptorSetLayout);
	}
}