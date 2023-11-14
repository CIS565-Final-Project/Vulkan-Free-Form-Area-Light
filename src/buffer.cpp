#include "buffer.h"

#include "device.h"
#include "commandPool.h"
#include "commandBuffer.h"
#include <iostream>

namespace VK_Renderer
{
	void VK_Buffer::FreeBuffer(vk::Device device, vk::Buffer buffer, vk::DeviceMemory deviceMemory)
	{
		if (buffer)
		{
			device.destroyBuffer(buffer);
		}
		if (deviceMemory)
		{
			device.freeMemory(deviceMemory);
		}
	}

	void VK_Buffer::CreateBuffer(VK_Device const & device,
								 vk::Buffer& buffer,
								 vk::DeviceMemory& deviceMemory,
								 vk::DeviceSize size,
								 vk::BufferUsageFlags usage,
								 vk::SharingMode sharingMode,
								 vk::MemoryPropertyFlags properties)
	{
		FreeBuffer(device.GetDevice(), buffer, deviceMemory);
		buffer = device.GetDevice().createBuffer(vk::BufferCreateInfo{
			.size = size,
			.usage = usage | vk::BufferUsageFlagBits::eTransferDst,
			.sharingMode = sharingMode
			});

		vk::MemoryRequirements mem_requirements = device.GetDevice().getBufferMemoryRequirements(buffer);

		deviceMemory = device.GetDevice().allocateMemory(vk::MemoryAllocateInfo{
			.allocationSize = mem_requirements.size,
			.memoryTypeIndex = device.GetMemoryTypeIndex(mem_requirements.memoryTypeBits, properties)
		});

		device.GetDevice().bindBufferMemory(buffer, deviceMemory, 0);
	}

	void VK_Buffer::UpdateBuffer(VK_Device const& device, 
								vk::Buffer buffer, void const* data, 
								vk::DeviceSize offset, 
								vk::DeviceSize size, 
								vk::SharingMode sharingMode)
	{
		vk::Buffer staging_buffer;
		vk::DeviceMemory staging_buffer_mem;

		CreateBuffer(device, staging_buffer, staging_buffer_mem, size, vk::BufferUsageFlagBits::eTransferSrc, sharingMode,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		void* mapped_data;
		vk::Result result = device.GetDevice().mapMemory(staging_buffer_mem, vk::DeviceSize(0), size, vk::MemoryMapFlags(), &mapped_data);
		if (result != vk::Result::eSuccess)
		{
			FreeBuffer(device.GetDevice(), staging_buffer, staging_buffer_mem);
			std::cerr << "Unabled to map host data!" << std::endl;
		}
		memcpy(mapped_data, data, size);
		device.GetDevice().unmapMemory(staging_buffer_mem);

		VK_CommandBuffer cmd = device.GetTransferCommandPool()->AllocateCommandBuffers();

		{
			cmd.Begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
			cmd[0].copyBuffer(staging_buffer, buffer, vk::BufferCopy{
				.srcOffset = 0,
				.dstOffset = offset,
				.size = size
				});
			cmd.End();
		}
		device.GetTransferQueue().submit(vk::SubmitInfo{
			.commandBufferCount = 1,
			.pCommandBuffers = &cmd[0]
		});
		device.GetTransferQueue().waitIdle();
		device.GetTransferCommandPool()->FreeCommandBuffer(cmd);

		FreeBuffer(device.GetDevice(), staging_buffer, staging_buffer_mem);
	}

	VK_Buffer::VK_Buffer(VK_Device const & device)
		: m_Device(device)
	{
	}
	

	void VK_Buffer::CreateFromData(void const* data,
									vk::DeviceSize size,
									vk::BufferUsageFlags usage,
									vk::SharingMode sharingMode,
									vk::MemoryPropertyFlags properties)
	{
		CreateBuffer(m_Device, vk_Buffer, vk_DeviceMemory, size, usage, sharingMode, properties);
		Update(data, 0, size, sharingMode);
	}

	void VK_Buffer::Update(void const* data,
							vk::DeviceSize offset,
							vk::DeviceSize size,
							vk::SharingMode sharingMode)
	{
		UpdateBuffer(m_Device, vk_Buffer, data, offset, size, sharingMode);
	}

	void VK_Buffer::Free() const
	{
		FreeBuffer(m_Device.GetDevice(), vk_Buffer, vk_DeviceMemory);
	}
}