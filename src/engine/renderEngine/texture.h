#pragma once

#include <vulkan/vulkan.hpp>

namespace VK_Renderer
{
	class VK_Device;
	class Image;

	struct VK_ImageLayout
	{
		vk::ImageLayout layout{ vk::ImageLayout::eUndefined };
		vk::AccessFlags accessFlag;
		vk::PipelineStageFlags pipelineStage;
		uint32_t queueFamily{ 0 };
	};

	struct TextureCreateInfo
	{
		vk::Format format;
		vk::ImageAspectFlags aspectMask{ vk::ImageAspectFlagBits::eColor };
		vk::ImageUsageFlags usage;
		vk::SampleCountFlagBits sampleCount{ vk::SampleCountFlagBits::e1 };
		vk::SharingMode sharingMode{ vk::SharingMode::eExclusive };
		uint32_t mipLevel{ 1 };
		uint32_t arrayLayer{ 1 };
	};

	class VK_Texture2D
	{
	public:
		VK_Texture2D(VK_Device const& device);
		~VK_Texture2D();

		void Create(vk::Extent3D const& extent, 
					TextureCreateInfo const& createInfo);

		void CreateFromFile(std::string const& file,
							TextureCreateInfo const& createInfo);
		void CreateFromImage(Image const& image, 
							TextureCreateInfo const& createInfo);

		void Free();

		void TransitionLayout(VK_ImageLayout const& targetLayout);

		void CopyFrom(void const* data, vk::Offset3D const& = {0, 0, 0});
		void CopyTo(void* data);

	protected:
		VK_Device const& m_Device;
		vk::UniqueDeviceMemory vk_DeviceMemory;
		vk::ImageSubresourceRange vk_SubresourceRange;

		VK_ImageLayout m_Layout;

		DeclareWithGetFunc(protected, vk::Format, vk, Format, const);
		DeclareWithGetFunc(protected, vk::DeviceSize, vk, Size, const);
		DeclareWithGetFunc(protected, vk::Extent3D, vk, Extent, const);
		DeclareWithGetFunc(protected, vk::Image, vk, Image, const);
		DeclareWithGetFunc(protected, vk::ImageView, vk, ImageView, const);
		DeclareWithGetFunc(protected, vk::Sampler, vk, Sampler, const);
	};

	class VK_Texture2DArray {
	public:
		VK_Texture2DArray(VK_Device const& device);
		~VK_Texture2DArray();

		void Create(vk::Extent3D const& extent,
			TextureCreateInfo const& createInfo);

		void CreateFromFiles(const std::vector<std::string>& file,
			TextureCreateInfo const& createInfo);
		void CreateFromImages(const std::vector<Image>& image,
			TextureCreateInfo const& createInfo);

		void Free();

		void TransitionLayout(VK_ImageLayout const& targetLayout);
		 
		void CopyFrom(void const* data, int width, int height, int channel = 4, vk::Offset3D const& = { 0, 0, 0 });
		void CopyTo(void* data);

	protected:
		VK_Device const& m_Device;
		vk::UniqueDeviceMemory vk_DeviceMemory;
		vk::ImageSubresourceRange vk_SubresourceRange;

		VK_ImageLayout m_Layout;

		DeclareWithGetFunc(protected, vk::Format, vk, Format, const);
		DeclareWithGetFunc(protected, vk::DeviceSize, vk, Size, const);
		DeclareWithGetFunc(protected, vk::Extent3D, vk, Extent, const);
		DeclareWithGetFunc(protected, vk::Image, vk, Image, const);
		DeclareWithGetFunc(protected, vk::ImageView, vk, ImageView, const);
		DeclareWithGetFunc(protected, vk::Sampler, vk, Sampler, const);
	};
}