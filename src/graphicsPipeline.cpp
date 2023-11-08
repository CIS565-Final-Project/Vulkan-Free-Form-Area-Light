#include "graphicsPipeline.h"

#include <fstream>

namespace VK_Renderer
{
	std::vector<char> ReadFile(const std::string& file)
	{
		std::ifstream in(file, std::ios::ate | std::ios::binary);
		std::vector<char> buffer;
		if (!in.is_open())
		{
			printf("Failed to Open %s", file.c_str());
			return buffer;
		}

		size_t file_size = in.tellg();
		in.seekg(0);
		buffer.resize(file_size);
		in.read(buffer.data(), file_size);

		return buffer;
	}

	VK_GraphicsPipeline::VK_GraphicsPipeline(VkDevice device, const VkExtent2D& extent, const VkFormat& swapchain_image_format)
		: m_LogicalDevice(device), 
		  m_SwapchainImageFormat(swapchain_image_format), 
		  m_Extent(extent)
	{
		// create the Renderpass before creating pipeline
		CreateRenderPass();

		auto vert_shader = ReadFile("shaders/flat.vert.spv");
		auto frag_shader = ReadFile("shaders/flat.frag.spv");

		m_VertShaderModule = CreateShaderModule(vert_shader);
		m_FragShaderModule = CreateShaderModule(frag_shader);

		// Vertex shader stage
		VkPipelineShaderStageCreateInfo vert_pipeline_stage_create_info = {};
		vert_pipeline_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_pipeline_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;

		vert_pipeline_stage_create_info.module = m_VertShaderModule;
		vert_pipeline_stage_create_info.pName  = "main";

		vert_pipeline_stage_create_info.pSpecializationInfo = nullptr;

		// Fragment shader stage
		VkPipelineShaderStageCreateInfo frag_pipeline_stage_create_info = {};
		frag_pipeline_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_pipeline_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

		frag_pipeline_stage_create_info.module = m_FragShaderModule;
		frag_pipeline_stage_create_info.pName = "main";

		frag_pipeline_stage_create_info.pSpecializationInfo = nullptr;

		std::vector<VkPipelineShaderStageCreateInfo> shader_stages 
		{
			vert_pipeline_stage_create_info, 
			frag_pipeline_stage_create_info
		};

		// Fixed-function stages

		// Vertex Input
		VkPipelineVertexInputStateCreateInfo vert_input_create_info = {};
		vert_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vert_input_create_info.vertexBindingDescriptionCount = 0;
		vert_input_create_info.pVertexBindingDescriptions = nullptr;
		vert_input_create_info.vertexAttributeDescriptionCount = 0;
		vert_input_create_info.pVertexAttributeDescriptions = nullptr;

		// Input Assembly
		VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {};
		input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

		// Set Viewport and scissors to be dynamic states that will be set when rendering
		std::vector<VkDynamicState> dynamic_states = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
		dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
		dynamic_state_create_info.pDynamicStates = dynamic_states.data();

		VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
		viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state_create_info.viewportCount = 1;
		viewport_state_create_info.scissorCount = 1;

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {};
		rasterizer_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer_create_info.depthClampEnable = VK_FALSE;
		rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;
		rasterizer_create_info.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer_create_info.lineWidth = 1.f;

		rasterizer_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;

		rasterizer_create_info.depthBiasEnable = VK_FALSE;
		rasterizer_create_info.depthBiasConstantFactor = 0.f;
		rasterizer_create_info.depthBiasClamp = 0.f;
		rasterizer_create_info.depthBiasSlopeFactor = 0.f;

		// Multisampling
		VkPipelineMultisampleStateCreateInfo multisample_create_info = {};
		multisample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_create_info.sampleShadingEnable = VK_FALSE;
		multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisample_create_info.minSampleShading = 1.f;
		multisample_create_info.alphaToOneEnable = VK_FALSE;
		multisample_create_info.alphaToCoverageEnable = VK_FALSE;

		// Color
		VkPipelineColorBlendAttachmentState color_blend_attachment = {};
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
												VK_COLOR_COMPONENT_G_BIT | 
												VK_COLOR_COMPONENT_B_BIT | 
												VK_COLOR_COMPONENT_A_BIT;

		color_blend_attachment.blendEnable = VK_FALSE;

		color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;

		color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

		VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
		color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_create_info.logicOpEnable = VK_FALSE;
		color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
		color_blend_create_info.attachmentCount = 1;
		color_blend_create_info.pAttachments = &color_blend_attachment;
		color_blend_create_info.blendConstants[0] = 0.f;
		color_blend_create_info.blendConstants[1] = 0.f;
		color_blend_create_info.blendConstants[2] = 0.f;
		color_blend_create_info.blendConstants[3] = 0.f;

		// Pipline Layout
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = 0;
		pipeline_layout_create_info.pSetLayouts = nullptr;
		pipeline_layout_create_info.pushConstantRangeCount = 0;
		pipeline_layout_create_info.pPushConstantRanges = nullptr;

		if(vkCreatePipelineLayout(m_LogicalDevice, &pipeline_layout_create_info, nullptr, &m_PipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Pipeline Layout!");
		}

		// Create Pipeline
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
		pipeline_create_info.pStages = shader_stages.data();
		pipeline_create_info.pVertexInputState = &vert_input_create_info;
		pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
		pipeline_create_info.pViewportState = &viewport_state_create_info;
		pipeline_create_info.pRasterizationState = &rasterizer_create_info;
		pipeline_create_info.pMultisampleState = &multisample_create_info;
		pipeline_create_info.pDepthStencilState = nullptr;
		pipeline_create_info.pColorBlendState = &color_blend_create_info;
		pipeline_create_info.pDynamicState = &dynamic_state_create_info;

		pipeline_create_info.layout = m_PipelineLayout;
		pipeline_create_info.renderPass = m_RenderPass;
		pipeline_create_info.subpass = 0;

		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
		pipeline_create_info.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &m_Pipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to Create Grpahics Pipeline!");
		}
	}

	VK_GraphicsPipeline::~VK_GraphicsPipeline()
	{
		vkDestroyPipeline(m_LogicalDevice, m_Pipeline, nullptr);
		vkDestroyRenderPass(m_LogicalDevice, m_RenderPass, nullptr);
		vkDestroyShaderModule(m_LogicalDevice, m_VertShaderModule, nullptr);
		vkDestroyShaderModule(m_LogicalDevice, m_FragShaderModule, nullptr);
		vkDestroyPipelineLayout(m_LogicalDevice, m_PipelineLayout, nullptr);
	}

	void VK_GraphicsPipeline::CreateRenderPass()
	{
		// Setup Framebuffer
		
		// color attachment 
		VkAttachmentDescription attachment_description = {};
		attachment_description.format = m_SwapchainImageFormat;
		attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		// attachment reference
		VkAttachmentReference attachment_reference = {};
		attachment_reference.attachment = 0;
		attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// Subpass
		VkSubpassDescription subpass_description = {};
		subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_description.colorAttachmentCount = 1;
		subpass_description.pColorAttachments = &attachment_reference;

		// Create RenderPass
		VkRenderPassCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		create_info.attachmentCount = 1;
		create_info.pAttachments = &attachment_description;
		create_info.subpassCount = 1;
		create_info.pSubpasses = &subpass_description;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		create_info.dependencyCount = 1;
		create_info.pDependencies = &dependency;

		if (vkCreateRenderPass(m_LogicalDevice, &create_info, nullptr, &m_RenderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to Create RenderPass!");
		}
	}

	VkShaderModule VK_GraphicsPipeline::CreateShaderModule(const std::vector<char>& source)
	{
		VkShaderModuleCreateInfo create_info = {};

		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = source.size();
		create_info.pCode = reinterpret_cast<const uint32_t*>(source.data());
		VkShaderModule shader_module;
		if (vkCreateShaderModule(m_LogicalDevice, &create_info, nullptr, &shader_module) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to Create Shader Module!");
		}
		return shader_module;
	}
}