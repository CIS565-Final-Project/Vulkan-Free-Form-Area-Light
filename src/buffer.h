#pragma once

#include <vulkan/vulkan.hpp>

namespace VK_Renderer
{
	class VK_Device;

	class VK_Buffer
	{
	public:
		VK_Buffer(VK_Device const & device);
		
		void CreateFromData(void const* data,
							vk::DeviceSize size,
							vk::BufferUsageFlags usage,
							vk::SharingMode sharingMode,
							vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eDeviceLocal);

		void Update(void const* data,
					vk::DeviceSize offset,
					vk::DeviceSize size,
					vk::SharingMode sharingMode);
		void Free() const;
	
	public:
		static void FreeBuffer(vk::Device device, vk::Buffer buffer, vk::DeviceMemory deviceMemory);

		static void CreateBuffer(VK_Device const & device,
								 vk::Buffer& buffer,
								 vk::DeviceMemory& deviceMemory,
								 vk::DeviceSize size,
								 vk::BufferUsageFlags usage,
								 vk::SharingMode sharingMode,
								 vk::MemoryPropertyFlags properties);

		static void UpdateBuffer(VK_Device const& device,
								  vk::Buffer buffer,
								  void const* data,
								  vk::DeviceSize offset,
								  vk::DeviceSize size,
								  vk::SharingMode sharingMode);

	protected:
		const VK_Device& m_Device;

		vk::DeviceMemory vk_DeviceMemory;
	public:
		vk::Buffer vk_Buffer;
	};
}