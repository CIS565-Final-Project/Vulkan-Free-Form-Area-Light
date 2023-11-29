#include "texture.h"

#include "device.h"
#include "commandPool.h"
#include "buffer.h"

#include "scene/image.h"
#include <iostream>

namespace VK_Renderer
{
	VK_Texture2D::VK_Texture2D(VK_Device const& device)
		: m_Device(device), 
		  m_Layout{.accessFlag = vk::AccessFlagBits::eNone, 
					.pipelineStage = vk::PipelineStageFlagBits::eTopOfPipe}
	{
	}
	VK_Texture2D::~VK_Texture2D()
	{
		Free();
	}

	void VK_Texture2D::Create(vk::Extent3D const& extent,
								TextureCreateInfo const& createInfo,
								uint32_t const& size)
	{
		Free();

		vk_Size = size;

		vk_Extent = extent;

		vk_Format = createInfo.format;

		vk_SubresourceRange = {
			.aspectMask = createInfo.aspectMask,
			.baseMipLevel = 0,
			.levelCount = createInfo.mipLevel,
			.baseArrayLayer = 0,
			.layerCount = createInfo.arrayLayer
		};

		vk_Image = m_Device.GetDevice().createImage(vk::ImageCreateInfo{
			.imageType = vk::ImageType::e2D,
			.format = vk_Format,
			.extent = vk_Extent,
			.mipLevels = vk_SubresourceRange.levelCount,
			.arrayLayers = vk_SubresourceRange.layerCount,
			.samples = createInfo.sampleCount,
			.tiling = vk::ImageTiling::eOptimal,
			.usage = createInfo.usage | vk::ImageUsageFlagBits::eTransferDst,
			.sharingMode = createInfo.sharingMode,
			.initialLayout = vk::ImageLayout::eUndefined,
		});

		vk_DeviceMemory = m_Device.AllocateImageMemory(vk_Image);

		m_Device.GetDevice().bindImageMemory(vk_Image, vk_DeviceMemory.get(), 0);

		// Create Image View
		vk_ImageView = m_Device.GetDevice().createImageView(vk::ImageViewCreateInfo{
			.image = vk_Image,
			.viewType = vk::ImageViewType::e2D,
			.format = vk_Format,
			.subresourceRange = vk_SubresourceRange,
		});
	}

	void VK_Texture2D::CreateFromFile(std::string const& file, 
									TextureCreateInfo const& createInfo)
	{
		CreateFromImage(Image{ file }, createInfo);
	}
	void VK_Texture2D::CreateFromImage(Image const& image, 
										TextureCreateInfo const& createInfo)
	{
		CreateFromData(image.GetRawData(), image.GetSize() , 
			{ 
				static_cast<uint32_t>(image.GetResolution().x),
				static_cast<uint32_t>(image.GetResolution().y),
				1
			}, createInfo);
	}

	void VK_Texture2D::CreateFromData(void* data, uint32_t const& size, vk::Extent3D const& extent, TextureCreateInfo const& createInfo)
	{
		Create(extent, createInfo, size);
		TransitionLayout(VK_ImageLayout{
			.layout = vk::ImageLayout::eTransferDstOptimal,
			.accessFlag = vk::AccessFlagBits::eMemoryWrite,
			.pipelineStage = vk::PipelineStageFlagBits::eTransfer,
			});
		CopyFrom(data);

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

	void VK_Texture2D::Free()
	{
		m_Layout = VK_ImageLayout{ .accessFlag = vk::AccessFlagBits::eNone,
									.pipelineStage = vk::PipelineStageFlagBits::eTopOfPipe };
		if (vk_Sampler) m_Device.GetDevice().destroySampler(vk_Sampler);
		if (vk_ImageView) m_Device.GetDevice().destroyImageView(vk_ImageView);
		if (vk_Image) m_Device.GetDevice().destroyImage(vk_Image);
	}

	void VK_Texture2D::TransitionLayout(VK_ImageLayout const& targetLayout)
	{
		VK_CommandBuffer cmd = m_Device.GetTransferCommandPool()->AllocateCommandBuffers();

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

		cmd.Begin({ .usage = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
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

		m_Layout = targetLayout;
	}

	void VK_Texture2D::CopyFrom(void const* data, vk::Offset3D const& offset)
	{
		VK_StagingBuffer staging_buffer(m_Device);
		staging_buffer.CreateFromData(data, vk_Size, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive);
		
		VK_CommandBuffer cmd = m_Device.GetTransferCommandPool()->AllocateCommandBuffers();
		cmd.Begin({ .usage = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

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
	}
	
	void VK_Texture2D::CopyTo(void* data)
	{
		
	}

	VK_Texture2DArray::VK_Texture2DArray(VK_Device const& device)
		: m_Device(device),
		m_Layout{ .accessFlag = vk::AccessFlagBits::eNone,
				  .pipelineStage = vk::PipelineStageFlagBits::eTopOfPipe }
	{}
	VK_Texture2DArray::~VK_Texture2DArray()
	{
		Free();
	}
	void VK_Texture2DArray::Create(vk::Extent3D const& extent, 
									TextureCreateInfo const& createInfo,
									uint32_t const& size)
	{
		Free();

		vk_Size = size;

		vk_Extent = extent;

		vk_Format = createInfo.format;

		vk_SubresourceRange = {
			.aspectMask = createInfo.aspectMask,
			.baseMipLevel = 0,
			.levelCount = createInfo.mipLevel,
			.baseArrayLayer = 0,
			.layerCount = createInfo.arrayLayer
		};

		vk_Image = m_Device.GetDevice().createImage(vk::ImageCreateInfo{
			.imageType = vk::ImageType::e2D,
			.format = vk_Format,
			.extent = vk_Extent,
			.mipLevels = vk_SubresourceRange.levelCount,
			.arrayLayers = vk_SubresourceRange.layerCount,
			.samples = createInfo.sampleCount,
			.tiling = vk::ImageTiling::eOptimal,
			.usage = createInfo.usage | vk::ImageUsageFlagBits::eTransferDst,
			.sharingMode = createInfo.sharingMode,
			.initialLayout = vk::ImageLayout::eUndefined,
			});

		vk_DeviceMemory = m_Device.AllocateImageMemory(vk_Image);

		m_Device.GetDevice().bindImageMemory(vk_Image, vk_DeviceMemory.get(), 0);

		// Create Image View
		vk_ImageView = m_Device.GetDevice().createImageView(vk::ImageViewCreateInfo{
			.image = vk_Image,
			.viewType = vk::ImageViewType::e2DArray,
			.format = vk_Format,
			.subresourceRange = vk_SubresourceRange,
			});
	}
	void VK_Texture2DArray::CreateFromFiles(const std::vector<std::string>& files, TextureCreateInfo const& createInfo)
	{
		std::vector<Image> images;
		for (const auto& file : files) {
			images.emplace_back(file);
		}
		CreateFromImages(images, createInfo);
	}
	void VK_Texture2DArray::CreateFromImages(const std::vector<Image>& images, TextureCreateInfo const& createInfo)
	{
		const auto& image = images[0];
		auto image_size = image.GetSize();
		for (const auto& tmp_image : images) {
			if (image_size != tmp_image.GetSize()) {
				std::cout << "size of images in texture2Darray should be the same!" << std::endl;
			}
		}

		Create({ static_cast<uint32_t>(image.GetResolution().x),
				 static_cast<uint32_t>(image.GetResolution().y),
				 1 
			}, createInfo, image.GetSize()* images.size());
		TransitionLayout(VK_ImageLayout{
			.layout = vk::ImageLayout::eTransferDstOptimal,
			.accessFlag = vk::AccessFlagBits::eMemoryWrite,
			.pipelineStage = vk::PipelineStageFlagBits::eTransfer,
			});
		
		void* tmp_raw_data = malloc(vk_Size);
		size_t offset = 0;
		for (size_t i = 0;i < images.size();++i) {
			std::memcpy((uint8_t*)tmp_raw_data + offset,images[i].GetRawData(), images[i].GetSize());
			offset += images[i].GetSize();
		}
		CopyFrom(tmp_raw_data,image.GetResolution().x, image.GetResolution().y);
		free(tmp_raw_data);

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
	void VK_Texture2DArray::Free()
	{
		m_Layout = VK_ImageLayout{ .accessFlag = vk::AccessFlagBits::eNone,
							.pipelineStage = vk::PipelineStageFlagBits::eTopOfPipe };
		if (vk_Sampler) m_Device.GetDevice().destroySampler(vk_Sampler);
		if (vk_ImageView) m_Device.GetDevice().destroyImageView(vk_ImageView);
		if (vk_Image) m_Device.GetDevice().destroyImage(vk_Image);
	}
	void VK_Texture2DArray::TransitionLayout(VK_ImageLayout const& targetLayout){
		VK_CommandBuffer cmd = m_Device.GetTransferCommandPool()->AllocateCommandBuffers();

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

		cmd.Begin({ .usage = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
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

		m_Layout = targetLayout;
	}
	void VK_Texture2DArray::CopyFrom(void const* data, int width, int height, int channel, vk::Offset3D const& offset) {
		VK_StagingBuffer staging_buffer(m_Device);
		staging_buffer.CreateFromData(data, vk_Size, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive);

		VK_CommandBuffer cmd = m_Device.GetTransferCommandPool()->AllocateCommandBuffers();

		cmd.Begin({ .usage = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		std::vector<VkBufferImageCopy> bufferCopyRegions;
		for (auto i = 0; i < vk_SubresourceRange.layerCount; ++i)
		{
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = i;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = width;
			bufferCopyRegion.imageExtent.height = height;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = width * height * channel * i;
			bufferCopyRegions.push_back(bufferCopyRegion);
		}
		cmd[0].copyBufferToImage(staging_buffer.GetBuffer(), vk_Image, m_Layout.layout, bufferCopyRegions.size(), (vk::BufferImageCopy*)bufferCopyRegions.data());
		
		cmd.End();

		m_Device.GetTransferQueue().submit(vk::SubmitInfo{
			.commandBufferCount = 1,
			.pCommandBuffers = &(cmd[0])
			});

		m_Device.GetTransferQueue().waitIdle();
		staging_buffer.Free();
	}
	//TODO
	void VK_Texture2DArray::CopyTo(void* data)
	{
	}
}