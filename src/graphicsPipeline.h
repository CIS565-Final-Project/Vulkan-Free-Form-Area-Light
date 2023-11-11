#pragma once

#include "camera.h"
#include "common.h"
#include "Model.h"

#include <vulkan/vulkan.h>

namespace VK_Renderer
{
	struct PipelineInfo
	{
		const char* vertShaderPath;
		const char* fragShaderPath;
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;
	};
	
	class VK_GraphicsPipeline
	{
	public:
		VK_GraphicsPipeline(VkDevice device, const VkExtent2D& extent, const VkFormat& swapchain_image_format,
			std::vector<Model*>& shadingModels, std::vector<Model*>& lightModels, Camera* camera);
		~VK_GraphicsPipeline();

	protected:
		void CreateRenderPass();
		void CreateStandardPipeline(PipelineInfo& pipelineInfo);
		void DestroyStandardPipeline(PipelineInfo& pipelineInfo);
		VkShaderModule CreateShaderModule(const std::vector<char>& source);
		void CreateModelDescriptorSets(std::vector<VkDescriptorSet>& modelDescriptorSets, std::vector<Model*>& models);
		void CreateModelDescriptorSetLayout();
		void CreateCameraDescriptorSet();
		void CreateCameraDescriptorSetLayout();
		void CreateDescriptorPool();

	public:
		PipelineInfo m_shadingPipelineInfo;
		PipelineInfo m_lightPipelineInfo;

		VkRenderPass m_RenderPass;

		VkDescriptorPool descriptorPool;

		VkDescriptorSet cameraDescriptorSet;
		VkDescriptorSetLayout modelDescriptorSetLayout;
		VkDescriptorSetLayout cameraDescriptorSetLayout;
		std::vector<VkDescriptorSet> shadingModelDescriptorSets;
		std::vector<VkDescriptorSet> lightModelDescriptorSets;

		std::vector<Model*>& m_shadingModels;
		std::vector<Model*>& m_lightModels;
		Camera* m_camera;

	protected:
		VkDevice m_LogicalDevice;
		VkFormat m_SwapchainImageFormat;
		VkExtent2D m_Extent;
	};
}