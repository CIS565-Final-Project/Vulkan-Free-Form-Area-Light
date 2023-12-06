#pragma once

#include <vulkan/vulkan.hpp>

namespace VK_Renderer
{
	class VK_Device;
	class VK_PipelineInput;
	class VK_RenderPass;

	struct GraphicspipelineCreateInfo
	{
		vk::RenderPass renderPass;
		uint32_t subpassIdx{ 0 };

	};

	struct MeshPipelineCreateInfo
	{
		std::vector<vk::DescriptorSetLayout> const& descriptorSetsLayout;
		std::string const& taskShaderPath{ "" };
		std::string const& meshShaderPath;
		std::string const& fragShaderPath;
	};

	class VK_GraphicsPipeline
	{
	public:
		VK_GraphicsPipeline(VK_Device const& device, VK_RenderPass const& renderPass);

		~VK_GraphicsPipeline();

		void Free();

		virtual void CreatePipeline(GraphicspipelineCreateInfo const& createInfo,
									const std::vector<vk::PipelineShaderStageCreateInfo>& pipelineShaderStagesCreateInfo,
									const VK_PipelineInput& pipelineInput, 
									std::vector<vk::DescriptorSetLayout> const& descripotrSetLayouts);

		void CreateMeshPipeline(MeshPipelineCreateInfo const& createInfo);

	protected:
		VK_Device const& m_Device;
		VK_RenderPass const& m_RenderPass;

		vk::UniquePipelineLayout vk_UniqueLayout;
		vk::UniquePipeline vk_UniquePipeline;

		DeclareWithGetFunc(protected, vk::PipelineLayout, vk, Layout, const);
		DeclareWithGetFunc(protected, vk::Pipeline, vk, Pipeline, const);
	};
}