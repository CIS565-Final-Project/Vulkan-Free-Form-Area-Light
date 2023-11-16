#include "graphicsPipeline.h"
#include <glm.hpp>

#include "device.h"
#include "pipelineInput.h"

namespace VK_Renderer
{
	VK_GraphicsPipeline::VK_GraphicsPipeline(const VK_Device& device,
											 const vk::Extent2D& extent,
											 const vk::Format& imageFormat)
		: m_Device(device),
		  vk_Extent(extent),
		  vk_SwapchainImageFormat(imageFormat)
	{
		// create the Renderpass before creating pipeline
		CreateRenderPass();
	}

	VK_GraphicsPipeline::~VK_GraphicsPipeline()
	{
		m_Device.GetDevice().destroyPipeline(vk_Pipeline);
		m_Device.GetDevice().destroyPipelineLayout(vk_PipelineLayout);
		m_Device.GetDevice().destroyRenderPass(vk_RenderPass);
	}

	void VK_GraphicsPipeline::CreatePipeline(const std::vector<vk::PipelineShaderStageCreateInfo>& pipelineShaderStagesCreateInfo, 
											const VK_PipelineInput& pipelineInput,
											std::vector<vk::DescriptorSetLayout> const& descripotrSetLayouts)
	{
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
			.frontFace = vk::FrontFace::eCounterClockwise,

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

		// PipelineLayout
		vk::PipelineLayoutCreateInfo pipeline_layout_create_info{
			.setLayoutCount = static_cast<uint32_t>(descripotrSetLayouts.size()),
			.pSetLayouts = descripotrSetLayouts.data()
		};

		vk_PipelineLayout = m_Device.GetDevice().createPipelineLayout(pipeline_layout_create_info);

		vk::GraphicsPipelineCreateInfo pipeline_create_info{
			.stageCount = static_cast<uint32_t>(pipelineShaderStagesCreateInfo.size()),
			.pStages = pipelineShaderStagesCreateInfo.data(),
			.pVertexInputState = &pipelineInput.GetPipeVertInputCreateInfo(),
			.pInputAssemblyState = &input_assembly_create_info,
			.pViewportState = &viewport_state_create_info,
			.pRasterizationState = &rasterization_create_info,
			.pMultisampleState = &multisample_create_info,
			.pColorBlendState = &color_blend_create_info,
			.pDynamicState = &dynamic_state_create_info,
			.layout = vk_PipelineLayout,
			.renderPass = vk_RenderPass,
			.subpass = 0,
		};

		// Create Pipeline
		vk::ResultValue<vk::Pipeline> result = m_Device.GetDevice().createGraphicsPipeline(vk::PipelineCache(), pipeline_create_info);
		if (result.result != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to create graphics pipeline");
		}
		vk_Pipeline = result.value;
	}

	void VK_GraphicsPipeline::CreateRenderPass()
	{
		// color attachment 
		vk::AttachmentDescription attachment_description{
			.format = vk_SwapchainImageFormat,
			.samples = vk::SampleCountFlagBits::e1,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.storeOp = vk::AttachmentStoreOp::eStore,
			.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
			.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
			.initialLayout = vk::ImageLayout::eUndefined,
			.finalLayout = vk::ImageLayout::ePresentSrcKHR,
		};

		// attachment reference
		vk::AttachmentReference attachment_ref{
			.attachment = 0,
			.layout = vk::ImageLayout::eColorAttachmentOptimal
		};

		// Subpass
		vk::SubpassDescription subpass_description{
			.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
			.colorAttachmentCount = 1,
			.pColorAttachments = &attachment_ref
		};

		vk::SubpassDependency dependency{
			.srcSubpass = vk::SubpassExternal,
			.dstSubpass = 0,
			.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
			.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
			.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
		};

		// Create RenderPass
		vk::RenderPassCreateInfo create_info{
			.attachmentCount = 1,
			.pAttachments = &attachment_description,
			.subpassCount = 1,
			.pSubpasses = &subpass_description,
			.dependencyCount = 1,
			.pDependencies = &dependency
		};
		vk_RenderPass = m_Device.GetDevice().createRenderPass(create_info);
	}
}