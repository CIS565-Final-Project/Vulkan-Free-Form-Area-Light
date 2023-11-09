#include "graphicsPipeline.h"
#include <glm.hpp>
#include <fstream>
#include "Camera.h"

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


	void VK_GraphicsPipeline::CreateDescriptorPool() {
		// Describe which descriptor types that the descriptor sets will contain
		std::vector<VkDescriptorPoolSize> poolSizes = {
			// Camera
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1},

			// Models + Blades
			// { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , static_cast<uint32_t>(m_models.size()) },

			// Models + Blades
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , static_cast<uint32_t>(m_models.size()) },
		};

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = 5;

		if (vkCreateDescriptorPool(m_LogicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor pool");
		}
	}

	void VK_GraphicsPipeline::CreateCameraDescriptorSetLayout() {
		// Describe the binding of the descriptor set layout
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding };

		// Create the descriptor set layout
		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(m_LogicalDevice, &layoutInfo, nullptr, &cameraDescriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout");
		}
	}

	void VK_GraphicsPipeline::CreateModelDescriptorSetLayout() {

		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		//VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
		//samplerLayoutBinding.binding = 1;
		//samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		//samplerLayoutBinding.descriptorCount = 1;
		//samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		//samplerLayoutBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding }; //samplerLayoutBinding

		// Create the descriptor set layout
		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(m_LogicalDevice, &layoutInfo, nullptr, &modelDescriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout");
		}
	}

	void VK_GraphicsPipeline::CreateCameraDescriptorSet() {
		// Describe the desciptor set
		VkDescriptorSetLayout layouts[] = { cameraDescriptorSetLayout };
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = layouts;

		// Allocate descriptor sets
		if (vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo, &cameraDescriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate descriptor set");
		}

		// Configure the descriptors to refer to buffers
		VkDescriptorBufferInfo cameraBufferInfo = {};
		cameraBufferInfo.buffer = m_camera->GetBuffer();
		cameraBufferInfo.offset = 0;
		cameraBufferInfo.range = sizeof(CameraBufferObject);

		std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = cameraDescriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &cameraBufferInfo;
		descriptorWrites[0].pImageInfo = nullptr;
		descriptorWrites[0].pTexelBufferView = nullptr;

		// Update descriptor sets
		vkUpdateDescriptorSets(m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}


	void VK_GraphicsPipeline::CreateModelDescriptorSets() {
		modelDescriptorSets.resize(m_models.size());

		// Describe the desciptor set
		// VkDescriptorSetLayout layouts[] = { modelDescriptorSetLayout };
		std::vector<VkDescriptorSetLayout> layouts = std::vector<VkDescriptorSetLayout>(m_models.size(), modelDescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(modelDescriptorSets.size());
		allocInfo.pSetLayouts = layouts.data();

		// Allocate descriptor sets
		if (vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo, modelDescriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate descriptor set");
		}

		std::vector<VkWriteDescriptorSet> descriptorWrites(modelDescriptorSets.size()); // 2 * 

		for (uint32_t i = 0; i < m_models.size(); ++i) {
			VkDescriptorBufferInfo modelBufferInfo = {};
			modelBufferInfo.buffer = m_models[i]->GetModelBuffer();
			modelBufferInfo.offset = 0;
			modelBufferInfo.range = sizeof(glm::mat4);

			// Bind image and sampler resources to the descriptor
			/*VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = m_models[i]->GetTextureView();
			imageInfo.sampler = m_models[i]->GetTextureSampler();

			descriptorWrites[2 * i + 0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2 * i + 0].dstSet = modelDescriptorSets[i];
			descriptorWrites[2 * i + 0].dstBinding = 0;
			descriptorWrites[2 * i + 0].dstArrayElement = 0;
			descriptorWrites[2 * i + 0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[2 * i + 0].descriptorCount = 1;
			descriptorWrites[2 * i + 0].pBufferInfo = &modelBufferInfo;
			descriptorWrites[2 * i + 0].pImageInfo = nullptr;
			descriptorWrites[2 * i + 0].pTexelBufferView = nullptr;*/

			descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[i].dstSet = modelDescriptorSets[i];
			descriptorWrites[i].dstBinding = 0;
			descriptorWrites[i].dstArrayElement = 0;
			descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[i].descriptorCount = 1;
			descriptorWrites[i].pBufferInfo = &modelBufferInfo;
			descriptorWrites[i].pImageInfo = nullptr;
			descriptorWrites[i].pTexelBufferView = nullptr;

			/*
			descriptorWrites[2 * i + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2 * i + 1].dstSet = modelDescriptorSets[i];
			descriptorWrites[2 * i + 1].dstBinding = 1;
			descriptorWrites[2 * i + 1].dstArrayElement = 0;
			descriptorWrites[2 * i + 1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[2 * i + 1].descriptorCount = 1;
			descriptorWrites[2 * i + 1].pImageInfo = &imageInfo;
			*/
		}

		// Update descriptor sets
		vkUpdateDescriptorSets(m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}


	VK_GraphicsPipeline::VK_GraphicsPipeline(VkDevice device, const VkExtent2D& extent, const VkFormat& swapchain_image_format, std::vector<Model*>& models, Camera* camera)
		: m_LogicalDevice(device), 
		  m_SwapchainImageFormat(swapchain_image_format), 
		  m_Extent(extent),
		  m_models(models),
		  m_camera(camera)
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

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vert_input_create_info.vertexBindingDescriptionCount = 1;
		vert_input_create_info.pVertexBindingDescriptions = &bindingDescription;
		vert_input_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vert_input_create_info.pVertexAttributeDescriptions = attributeDescriptions.data();

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


		CreateCameraDescriptorSetLayout();
		CreateModelDescriptorSetLayout();
		CreateDescriptorPool();
		CreateCameraDescriptorSet();
		CreateModelDescriptorSets();		

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { cameraDescriptorSetLayout, modelDescriptorSetLayout };

		// Pipline Layout
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipeline_layout_create_info.pSetLayouts = descriptorSetLayouts.data();
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
		vkDestroyDescriptorSetLayout(m_LogicalDevice, modelDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_LogicalDevice, cameraDescriptorSetLayout, nullptr);

		vkDestroyDescriptorPool(m_LogicalDevice, descriptorPool, nullptr);
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