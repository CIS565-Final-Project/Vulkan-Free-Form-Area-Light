#pragma once

#include <vulkan/vulkan.hpp>

namespace VK_Renderer
{
	class VK_Device;

	class VK_GraphicsPipeline
	{
	public:
		VK_GraphicsPipeline(const VK_Device& device,
							const vk::Extent2D& extent, 
							const vk::Format& imageFormat);

		~VK_GraphicsPipeline();

		virtual void CreatePipeline(const std::vector<vk::PipelineShaderStageCreateInfo>& pipelineShaderStagesCreateInfo);

	protected:
		void CreateRenderPass();

	public:
		vk::RenderPass vk_RenderPass;
		vk::PipelineLayout vk_PipelineLayout;
		vk::Pipeline vk_Pipeline;

	protected:
		const VK_Device& m_Device;

		const vk::Format& vk_SwapchainImageFormat;
		const vk::Extent2D& vk_Extent;
	};
}