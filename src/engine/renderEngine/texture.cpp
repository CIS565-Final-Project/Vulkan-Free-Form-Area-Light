#include "texture.h"

#include "device.h"
#include "commandPool.h"
#include "buffer.h"

#include "scene/image.h"

namespace VK_Renderer
{
	VK_Texture::VK_Texture(VK_Device const& device)
		: m_Device(device), 
		  m_Layout{.accessFlag = vk::AccessFlagBits::eNone, 
					.pipelineStage = vk::PipelineStageFlagBits::eTopOfPipe}
	{
	}
	VK_Texture::~VK_Texture()
	{
		Free();
	}
	void VK_Texture::CreateFromFile(std::string const& file, 
									uint32_t const& mipLevels,
									vk::Format format,
									vk::ImageUsageFlags usage,
									vk::SharingMode sharingMode)
	{
		Image image;
		image.LoadFromFile(file);
		CreateFromImage(image, mipLevels, format, usage, sharingMode);
	}
	void VK_Texture::CreateFromImage(Image const& image, 
									 uint32_t const& mipLevels,
									 vk::Format format,
									 vk::ImageUsageFlags usage,
									 vk::SharingMode sharingMode)
	{
		vk_Extent = { static_cast<uint32_t>(image.GetResolution().x),
					  static_cast<uint32_t>(image.GetResolution().y), 
					1 };

		vk_Size = image.GetSize();

		vk_Format = format;

		vk_SubresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		};

		vk_Image = m_Device.GetDevice().createImage(vk::ImageCreateInfo{
			.imageType = vk::ImageType::e2D,
			.format = format,
			.extent = vk_Extent,
			.mipLevels = mipLevels,
			.arrayLayers = 1,
			.samples = vk::SampleCountFlagBits::e1,
			.tiling = vk::ImageTiling::eOptimal,
			.usage = usage | vk::ImageUsageFlagBits::eTransferDst,
			.sharingMode = sharingMode,
			.initialLayout = vk::ImageLayout::eUndefined,
		});

		vk_DeviceMemory = m_Device.AllocateImageMemory(vk_Image);

		m_Device.GetDevice().bindImageMemory(vk_Image, vk_DeviceMemory.get(), 0);

		TransitionLayout(VK_ImageLayout{
			.layout = vk::ImageLayout::eTransferDstOptimal,
			.accessFlag = vk::AccessFlagBits::eMemoryWrite,
			.pipelineStage = vk::PipelineStageFlagBits::eTransfer,
		});
		CopyFrom(image.GetRawData());

		// Create Image View
		vk_ImageView = m_Device.GetDevice().createImageView(vk::ImageViewCreateInfo{
			.image = vk_Image,
			.viewType = vk::ImageViewType::e2D,
			.format = vk_Format,
			.subresourceRange = vk_SubresourceRange,
		});

		vk::PhysicalDeviceProperties property = m_Device.GetPhysicalDevice().getProperties();

		// Create Sampler
		vk_Sampler = m_Device.GetDevice().createSampler(vk::SamplerCreateInfo{
			.magFilter = vk::Filter::eLinear,
			.minFilter = vk::Filter::eLinear,
			.mipmapMode = vk::SamplerMipmapMode::eLinear,
			.addressModeU = vk::SamplerAddressMode::eRepeat,
			.addressModeV = vk::SamplerAddressMode::eRepeat,
			.addressModeW = vk::SamplerAddressMode::eRepeat,
			.mipLodBias = 0.f,
			.anisotropyEnable = vk::True,
			.maxAnisotropy = property.limits.maxSamplerAnisotropy,
			.compareEnable = vk::False,
			.minLod = 0.f,
			.maxLod = 0.f,
			.borderColor = vk::BorderColor::eIntOpaqueBlack,
			.unnormalizedCoordinates = vk::False
		});
	}

	void VK_Texture::Free()
	{
		if (vk_Sampler) m_Device.GetDevice().destroySampler(vk_Sampler);
		if (vk_ImageView) m_Device.GetDevice().destroyImageView(vk_ImageView);
		if (vk_Image) m_Device.GetDevice().destroyImage(vk_Image);
	}

	void VK_Texture::TransitionLayout(VK_ImageLayout const& targetLayout)
	{
		uPtr<VK_CommandBuffer> cmd_ptr = m_Device.GetTransferCommandPool()->AllocateCommandBuffers();

		vk::ImageMemoryBarrier barrier{
			.srcAccessMask = m_Layout.accessFlag,
			.dstAccessMask = targetLayout.accessFlag,
			.oldLayout = m_Layout.layout,
			.newLayout = targetLayout.layout,
			.srcQueueFamilyIndex = m_Layout.queueFamily,
			.dstQueueFamilyIndex = targetLayout.queueFamily,
			.image = vk_Image,
			.subresourceRange = vk_SubresourceRange,
			
		};

		VK_CommandBuffer& cmd = *cmd_ptr;
		cmd.Begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		cmd[0].pipelineBarrier(m_Layout.pipelineStage, 
								targetLayout.pipelineStage, 
								vk::DependencyFlags{}, nullptr, nullptr, 
								barrier);
		cmd.End();
		m_Device.GetTransferQueue().submit(vk::SubmitInfo{
			.commandBufferCount = 1,
			.pCommandBuffers = &(cmd[0])
		});
		
		m_Device.GetTransferQueue().waitIdle();

		m_Device.GetTransferCommandPool()->FreeCommandBuffer(*cmd_ptr);

		m_Layout = targetLayout;
	}

	void VK_Texture::CopyFrom(void const* data, vk::Offset3D const& offset)
	{
		VK_StagingBuffer staging_buffer(m_Device);
		staging_buffer.CreateFromData(data, vk_Size, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive);
		
		uPtr<VK_CommandBuffer> cmd_ptr = m_Device.GetTransferCommandPool()->AllocateCommandBuffers();
		
		VK_CommandBuffer& cmd = *cmd_ptr;

		cmd.Begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		cmd[0].copyBufferToImage(staging_buffer.GetBuffer(), vk_Image, m_Layout.layout, vk::BufferImageCopy{
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = vk::ImageSubresourceLayers{
				.aspectMask = vk_SubresourceRange.aspectMask,
				.mipLevel = 0,
				.baseArrayLayer = vk_SubresourceRange.baseArrayLayer,
				.layerCount = vk_SubresourceRange.layerCount
			},
			.imageOffset = offset,
			.imageExtent = vk_Extent
		});
		cmd.End();
		
		m_Device.GetTransferQueue().submit(vk::SubmitInfo{
			.commandBufferCount = 1,
			.pCommandBuffers = &(cmd[0])
		});

		m_Device.GetTransferQueue().waitIdle();
		staging_buffer.Free();
		m_Device.GetTransferCommandPool()->FreeCommandBuffer(*cmd_ptr);
	}
	
	void VK_Texture::CopyTo(void* data)
	{
		
	}
}