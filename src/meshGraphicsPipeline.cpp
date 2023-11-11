#include "meshGraphicsPipeline.h"

namespace VK_Renderer
{
	VK_MeshGraphicsPipeline::VK_MeshGraphicsPipeline(VkDevice device, const VkExtent2D& extent, const VkFormat& swapchain_image_format, std::vector<Model*>& models, Camera* camera)
		:VK_GraphicsPipeline(device, extent, swapchain_image_format, models, camera)
	{
	}
	VK_MeshGraphicsPipeline::~VK_MeshGraphicsPipeline()
	{
		//vk_LogicalDevice.destroyPipelineLayout(vk_PipelineLayout);
		//vk_LogicalDevice.destroyPipeline(vk_Pipeline);
	}
	void VK_MeshGraphicsPipeline::CreatePipeline(const std::vector<vk::PipelineShaderStageCreateInfo>& pipelineShaderStagesCreateInfo)
	{
		// Vertex Input
		vk::PipelineVertexInputStateCreateInfo vert_input_create_info{
			.vertexBindingDescriptionCount = 0,
			.vertexAttributeDescriptionCount = 0
		};

		// Input Assembly
		vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info{
			.topology = vk::PrimitiveTopology::eTriangleList,
			.primitiveRestartEnable = vk::False
		};

		// Set Viewport and scissors to be dynamic states that will be set when rendering
		std::vector<vk::DynamicState> dynamic_states = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor,
		};

		vk::PipelineDynamicStateCreateInfo dynamic_state_create_info{
			.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
			.pDynamicStates = dynamic_states.data()
		};
		vk::PipelineViewportStateCreateInfo viewport_state_create_info{
			.viewportCount = 1,
			.scissorCount = 1
		};

		// Rasterizer
		vk::PipelineRasterizationStateCreateInfo rasterization_create_info{
			.depthClampEnable = vk::False,
			.rasterizerDiscardEnable = vk::False,
			.polygonMode = vk::PolygonMode::eFill,
			
			.cullMode = vk::CullModeFlagBits::eBack,
			.frontFace = vk::FrontFace::eClockwise,

			.depthBiasEnable = vk::False,
			.depthBiasConstantFactor = 0.f,
			.depthBiasClamp = 0.f,
			.depthBiasSlopeFactor = 0.f,
			.lineWidth = 1.f,
		};
		// Multisampling
		vk::PipelineMultisampleStateCreateInfo multisample_create_info{
			.rasterizationSamples = vk::SampleCountFlagBits::e1,
			.sampleShadingEnable = vk::False,
			.minSampleShading = 1.f,
			.alphaToCoverageEnable = vk::False,
			.alphaToOneEnable = vk::False
		};

		vk::PipelineColorBlendAttachmentState color_blend_attachment{
			.blendEnable = vk::False,
			
			.srcColorBlendFactor = vk::BlendFactor::eOne,
			.dstColorBlendFactor = vk::BlendFactor::eZero,
			.colorBlendOp = vk::BlendOp::eAdd,

			.srcAlphaBlendFactor = vk::BlendFactor::eOne,
			.dstAlphaBlendFactor = vk::BlendFactor::eZero,
			.alphaBlendOp = vk::BlendOp::eAdd,

			.colorWriteMask = vk::ColorComponentFlagBits::eR |
								vk::ColorComponentFlagBits::eG |
								vk::ColorComponentFlagBits::eB |
								vk::ColorComponentFlagBits::eA,
		};

		vk::PipelineColorBlendStateCreateInfo color_blend_create_info{
			.logicOpEnable = vk::False,
			.logicOp = vk::LogicOp::eCopy,
			.attachmentCount = 1,
			.pAttachments = &color_blend_attachment,
			.blendConstants = std::array<float, 4>{0.f, 0.f, 0.f, 0.f}
		};

		vk::PipelineLayoutCreateInfo pipeline_layout_create_info{};

		vk_PipelineLayout = vk_LogicalDevice.createPipelineLayout(pipeline_layout_create_info);
		m_PipelineLayout = vk_PipelineLayout;

		vk::GraphicsPipelineCreateInfo pipeline_create_info{
			.stageCount = static_cast<uint32_t>(pipelineShaderStagesCreateInfo.size()),
			.pStages = pipelineShaderStagesCreateInfo.data(),
			.pVertexInputState = &vert_input_create_info,
			.pInputAssemblyState = &input_assembly_create_info,
			.pViewportState = &viewport_state_create_info,
			.pRasterizationState = &rasterization_create_info,
			.pMultisampleState = &multisample_create_info,
			.pColorBlendState = &color_blend_create_info,
			.pDynamicState = &dynamic_state_create_info,
			.layout = vk_PipelineLayout,
			.renderPass = m_RenderPass,
			.subpass = 0,
		};

		// Create Pipeline
		vk::ResultValue<vk::Pipeline> result = vk_LogicalDevice.createGraphicsPipeline(vk::PipelineCache(), pipeline_create_info);
		if (result.result != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to create graphics pipeline");
		}
		vk_Pipeline = result.value;
		m_Pipeline = vk_Pipeline;
	}
}