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

	class VK_Texture
	{
	public:
		VK_Texture(VK_Device const& device);
		~VK_Texture();

		void CreateFromFile(std::string const& file,
							uint32_t const& mipLevels,
							vk::Format format,
							vk::ImageUsageFlags usage,
							vk::SharingMode sharingMode);
		void CreateFromImage(Image const& image, 
							uint32_t const& mipLevels,
							vk::Format format, 
							vk::ImageUsageFlags usage,
							vk::SharingMode sharingMode);

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
}