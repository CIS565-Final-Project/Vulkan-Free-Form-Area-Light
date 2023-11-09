#pragma once

#include "camera.h"
#include "common.h"
#include "Model.h"

#include <vulkan/vulkan.h>

namespace VK_Renderer
{
	class VK_GraphicsPipeline
	{
	public:
		VK_GraphicsPipeline(VkDevice device, const VkExtent2D& extent, const VkFormat& swapchain_image_format, std::vector<Model*>& models, Camera* camera);
		~VK_GraphicsPipeline();

	protected:
		void CreateRenderPass();
		VkShaderModule CreateShaderModule(const std::vector<char>& source);
		void CreateModelDescriptorSets();
		void CreateModelDescriptorSetLayout();
		void CreateCameraDescriptorSet();
		void CreateCameraDescriptorSetLayout();
		void CreateDescriptorPool();

	public:
		VkShaderModule m_VertShaderModule;
		VkShaderModule m_FragShaderModule;
		VkRenderPass m_RenderPass;
		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_Pipeline;

		VkDescriptorPool descriptorPool;

		std::vector<VkDescriptorSet> modelDescriptorSets;
		VkDescriptorSet cameraDescriptorSet;
		VkDescriptorSetLayout modelDescriptorSetLayout;
		VkDescriptorSetLayout cameraDescriptorSetLayout;
		std::vector<Model*>& m_models;
		Camera* m_camera;

	protected:
		VkDevice m_LogicalDevice;
		VkFormat m_SwapchainImageFormat;
		VkExtent2D m_Extent;
	};
}