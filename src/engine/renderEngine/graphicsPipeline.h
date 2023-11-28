#pragma once

#include <vulkan/vulkan.hpp>

namespace VK_Renderer
{
	class VK_Device;
	class VK_PipelineInput;

	struct GraphicspipelineCreateInfo
	{
		vk::RenderPass renderPass;
		uint32_t subpassIdx{ 0 };

	};

	class VK_GraphicsPipeline
	{
	public:
		VK_GraphicsPipeline(const VK_Device& device);

		~VK_GraphicsPipeline();

		void Free();

		virtual void CreatePipeline(GraphicspipelineCreateInfo const& createInfo,
									const std::vector<vk::PipelineShaderStageCreateInfo>& pipelineShaderStagesCreateInfo,
									const VK_PipelineInput& pipelineInput, 
									std::vector<vk::DescriptorSetLayout> const& descripotrSetLayouts);

	protected:
		const VK_Device& m_Device;

		vk::UniquePipelineLayout vk_UniqueLayout;
		vk::UniquePipeline vk_UniquePipeline;

		DeclareWithGetFunc(protected, vk::PipelineLayout, vk, Layout, const);
		DeclareWithGetFunc(protected, vk::Pipeline, vk, Pipeline, const);
	};
}