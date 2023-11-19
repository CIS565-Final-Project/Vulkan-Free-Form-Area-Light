#pragma once

#include <vulkan/vulkan.hpp>

namespace VK_Renderer
{
	class VK_Device;

	class VK_Buffer
	{
	public:
		VK_Buffer(VK_Device const & device);
		virtual ~VK_Buffer();

		virtual void Create(vk::DeviceSize size,
			vk::BufferUsageFlags usage,
			vk::SharingMode sharingMode) = 0;

		virtual void CreateFromData(void const* data,
			vk::DeviceSize size,
			vk::BufferUsageFlags usage,
			vk::SharingMode sharingMode) =0;

		virtual void Update(void const* data,
			vk::DeviceSize offset,
			vk::DeviceSize size) = 0;

		virtual void Free();
	
	protected:
		static void FreeBuffer(vk::Device device, vk::Buffer& buffer, vk::DeviceMemory& deviceMemory);

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
								  vk::DeviceSize size);

	protected:
		const VK_Device& m_Device;

		vk::DeviceMemory vk_DeviceMemory;
		DeclareWithGetFunc(protected, vk::DeviceSize, vk, Size, const);
		DeclareWithGetFunc(protected, vk::Buffer, vk, Buffer, const);
	};

	class VK_StagingBuffer : public VK_Buffer
	{
	public:
		VK_StagingBuffer(VK_Device const& device);

		virtual void Create(vk::DeviceSize size,
							vk::BufferUsageFlags usage,
							vk::SharingMode sharingMode);

		virtual void CreateFromData(void const* data,
									vk::DeviceSize size,
									vk::BufferUsageFlags usage,
									vk::SharingMode sharingMode);

		virtual void Update(void const* data,
							vk::DeviceSize offset,
							vk::DeviceSize size);

		virtual void Free();

	protected:
		void* m_MappedMemory;
	};

	class VK_DeviceBuffer : public VK_Buffer
	{
	public:
		VK_DeviceBuffer(VK_Device const& device);

		virtual void Create(vk::DeviceSize size,
							vk::BufferUsageFlags usage,
							vk::SharingMode sharingMode);

		virtual void CreateFromData(void const* data,
									vk::DeviceSize size,
									vk::BufferUsageFlags usage,
									vk::SharingMode sharingMode);

		virtual void Update(void const* data,
							vk::DeviceSize offset,
							vk::DeviceSize size);
	};
}