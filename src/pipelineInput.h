#pragma once

#include <vulkan/vulkan.hpp>

namespace VK_Renderer
{
	class VK_PipelineInput
	{
	public:
		virtual void SetupPipelineVertexInputCreateInfo() {}

		DeclareWithGetFunc(protected, vk::PipelineVertexInputStateCreateInfo, vk, PipeVertInputCreateInfo, const);
	};

	class VK_GeneralPipeInput : public VK_PipelineInput
	{
	public:
		virtual void SetupPipelineVertexInputCreateInfo() override;

	protected:
		vk::VertexInputBindingDescription					vk_VertInputBindingDesc;
		std::array<vk::VertexInputAttributeDescription, 3>	vk_VertInputAttrDescs;
	};
}