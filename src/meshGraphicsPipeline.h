#pragma once

#include "graphicsPipeline.h"

namespace VK_Renderer
{
	class VK_MeshGraphicsPipeline : public VK_GraphicsPipeline
	{
	public:
		VK_MeshGraphicsPipeline(VkDevice device, const VkExtent2D& extent, const VkFormat& swapchain_image_format, std::vector<Model*>& models, Camera* camera);
		~VK_MeshGraphicsPipeline();

		virtual void CreatePipeline(const std::vector<vk::PipelineShaderStageCreateInfo>& pipelineShaderStagesCreateInfo) override;

	public:
		vk::PipelineLayout vk_PipelineLayout;
		vk::Pipeline vk_Pipeline;
	};
}