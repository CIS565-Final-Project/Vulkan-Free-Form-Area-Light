#include "pipelineInput.h"

#include "vertex.h"
#include "vulkanUtils.h"

namespace VK_Renderer
{
	void VK_GeneralPipeInput::SetupPipelineVertexInputCreateInfo()
	{
		vk_VertInputBindingDesc = {
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = vk::VertexInputRate::eVertex
		};

		vk_VertInputAttrDescs = {
			vk::VertexInputAttributeDescription{
				.location = 0,
				.binding = 0,
				.format = vk_FormatFloat3
			},
			vk::VertexInputAttributeDescription{
				.location = 1,
				.binding = 0,
				.format = vk_FormatFloat3
			},
			vk::VertexInputAttributeDescription{
				.location = 2,
				.binding = 0,
				.format = vk_FormatFloat2
			}
		};

		vk_PipeVertInputCreateInfo = {
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &vk_VertInputBindingDesc,
			.vertexAttributeDescriptionCount = 3,
			.pVertexAttributeDescriptions = vk_VertInputAttrDescs.data()
		};
	}
}
