#include "graphicsPipeline.h"
#include <glm.hpp>

#include "device.h"
#include "pipelineInput.h"
#include "renderPass.h"

namespace VK_Renderer
{
	VK_GraphicsPipeline::VK_GraphicsPipeline(const VK_Device& device, VK_RenderPass const& renderPass)
		: m_Device(device), m_RenderPass(renderPass)
	{
	}

	VK_GraphicsPipeline::~VK_GraphicsPipeline()
	{
		Free();
	}

	void VK_GraphicsPipeline::Free()
	{
		vk_UniqueLayout.reset();
		vk_UniquePipeline.reset();
	}

	void VK_GraphicsPipeline::CreatePipeline(GraphicspipelineCreateInfo const& createInfo, 
											const std::vector<vk::PipelineShaderStageCreateInfo>& pipelineShaderStagesCreateInfo,
											const VK_PipelineInput& pipelineInput,
											std::vector<vk::DescriptorSetLayout> const& descripotrSetLayouts)
	{
		// Delete exisit layout and pipeline 
		Free();

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
			.rasterizationSamples = m_Device.GetDeviceProperties().maxSampleCount,
			.sampleShadingEnable = vk::False,
			.minSampleShading = 1.f,
			.alphaToCoverageEnable = vk::False,
			.alphaToOneEnable = vk::False
		};

		// Depth Stencil State
		vk::PipelineDepthStencilStateCreateInfo depth_stencil_create_info{
			.depthTestEnable = vk::True,
			.depthWriteEnable = vk::True,
			.depthCompareOp = vk::CompareOp::eLess,
			.depthBoundsTestEnable = vk::False,
			.stencilTestEnable = vk::False
		};

		// Color Blend State
		vk::PipelineColorBlendAttachmentState color_blend_attachment{
			.blendEnable = vk::True,

			.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
			.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
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

		vk_UniqueLayout = m_Device.GetDevice().createPipelineLayoutUnique(pipeline_layout_create_info);
		vk_Layout = vk_UniqueLayout.get();

		vk::GraphicsPipelineCreateInfo pipeline_create_info{
			.stageCount = static_cast<uint32_t>(pipelineShaderStagesCreateInfo.size()),
			.pStages = pipelineShaderStagesCreateInfo.data(),
			.pVertexInputState = &pipelineInput.GetPipeVertInputCreateInfo(),
			.pInputAssemblyState = &input_assembly_create_info,
			.pViewportState = &viewport_state_create_info,
			.pRasterizationState = &rasterization_create_info,
			.pMultisampleState = &multisample_create_info,
			.pDepthStencilState = &depth_stencil_create_info,
			.pColorBlendState = &color_blend_create_info,
			.pDynamicState = &dynamic_state_create_info,
			.layout = vk_Layout,
			.renderPass = createInfo.renderPass,
			.subpass = createInfo.subpassIdx
		};

		// Create Pipeline
		vk::ResultValue<vk::UniquePipeline> result = m_Device.GetDevice().createGraphicsPipelineUnique(vk::PipelineCache(), pipeline_create_info);
		if (result.result != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to create graphics pipeline");
		}
		vk_UniquePipeline = std::move(result.value);
		vk_Pipeline = vk_UniquePipeline.get();
	}
	void VK_GraphicsPipeline::CreateMeshPipeline(MeshPipelineCreateInfo const& createInfo)
	{
		// Create required shader stages
		auto task_shader = ReadFile(createInfo.taskShaderPath);
		auto mesh_shader = ReadFile(createInfo.meshShaderPath);
		auto frag_shader = ReadFile(createInfo.fragShaderPath);

		vk::UniqueShaderModule task_module = m_Device.GetDevice().createShaderModuleUnique(vk::ShaderModuleCreateInfo{
			.codeSize = task_shader.size(),
			.pCode = reinterpret_cast<const uint32_t*>(task_shader.data())
			});
		vk::UniqueShaderModule mesh_module = m_Device.GetDevice().createShaderModuleUnique(vk::ShaderModuleCreateInfo{
			.codeSize = mesh_shader.size(),
			.pCode = reinterpret_cast<const uint32_t*>(mesh_shader.data())
			});
		vk::UniqueShaderModule frag_module = m_Device.GetDevice().createShaderModuleUnique(vk::ShaderModuleCreateInfo{
			.codeSize = frag_shader.size(),
			.pCode = reinterpret_cast<const uint32_t*>(frag_shader.data())
			});

		std::vector<vk::PipelineShaderStageCreateInfo> shader_stages
		{
			vk::PipelineShaderStageCreateInfo{
				.stage = vk::ShaderStageFlagBits::eTaskEXT,
				.module = task_module.get(),
				.pName = "main"
			},
			vk::PipelineShaderStageCreateInfo{
				.stage = vk::ShaderStageFlagBits::eMeshEXT,
				.module = mesh_module.get(),
				.pName = "main"
			},
			vk::PipelineShaderStageCreateInfo{
				.stage = vk::ShaderStageFlagBits::eFragment,
				.module = frag_module.get(),
				.pName = "main"
			},
		};

		VK_PipelineInput input;
		input.SetupPipelineVertexInputCreateInfo();

		CreatePipeline({ .renderPass = m_RenderPass.GetRenderPass() }, shader_stages, input, createInfo.descriptorSetsLayout);
	}
}