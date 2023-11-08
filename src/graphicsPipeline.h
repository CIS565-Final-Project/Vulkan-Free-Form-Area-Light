#pragma once

#include <vulkan/vulkan.h>

#include "common.h"

namespace VK_Renderer
{
	class VK_GraphicsPipeline
	{
	public:
		VK_GraphicsPipeline(VkDevice device, const VkExtent2D& extent, const VkFormat& swapchain_image_format);
		~VK_GraphicsPipeline();

	protected:
		void CreateRenderPass();
		VkShaderModule CreateShaderModule(const std::vector<char>& source);
	public:
		VkShaderModule m_VertShaderModule;
		VkShaderModule m_FragShaderModule;
		VkRenderPass m_RenderPass;
		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_Pipeline;

	protected:
		VkDevice m_LogicalDevice;
		VkFormat m_SwapchainImageFormat;
		VkExtent2D m_Extent;
	};
}