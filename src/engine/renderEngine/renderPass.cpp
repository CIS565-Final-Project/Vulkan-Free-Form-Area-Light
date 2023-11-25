#include "renderPass.h"

#include "device.h"

namespace VK_Renderer
{
	VK_RenderPass::VK_RenderPass(VK_Device const& device)
		: m_Device(device)
	{
	}

	void VK_RenderPass::Create(vk::Format const& swapchainImageFormat)
	{
		// attachments
		std::vector<vk::AttachmentDescription> attachments{
			// Resolve Attachments (Present to Screen)
			{
				.format = swapchainImageFormat,
				.samples = vk::SampleCountFlagBits::e1,
				.loadOp = vk::AttachmentLoadOp::eClear,
				.storeOp = vk::AttachmentStoreOp::eStore,
				.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
				.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
				.initialLayout = vk::ImageLayout::eUndefined,
				.finalLayout = vk::ImageLayout::ePresentSrcKHR,
			},
			// Depth Attachments
			{
				.format = vk::Format::eD32Sfloat,
				.samples = m_Device.GetDeviceProperties().maxSampleCount,
				.loadOp = vk::AttachmentLoadOp::eClear,
				.storeOp = vk::AttachmentStoreOp::eDontCare,
				.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
				.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
				.initialLayout = vk::ImageLayout::eUndefined,
				.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
			},
			// Color Attachments
			{
				.format = swapchainImageFormat,
				.samples = m_Device.GetDeviceProperties().maxSampleCount,
				.loadOp = vk::AttachmentLoadOp::eClear,
				.storeOp = vk::AttachmentStoreOp::eStore,
				.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
				.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
				.initialLayout = vk::ImageLayout::eUndefined,
				.finalLayout = vk::ImageLayout::eColorAttachmentOptimal,
			}
		};

		// attachment reference
		vk::AttachmentReference resolve_attachment_ref{
			.attachment = 0,
			.layout = vk::ImageLayout::eColorAttachmentOptimal
		};
		vk::AttachmentReference depth_attachment_ref{
			.attachment = 1,
			.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
		};

		vk::AttachmentReference color_Attachment_ref{
			.attachment = 2,
			.layout = vk::ImageLayout::eColorAttachmentOptimal
		};

		// Subpass
		vk::SubpassDescription subpass_description{
			.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_Attachment_ref,
			.pResolveAttachments = &resolve_attachment_ref,
			.pDepthStencilAttachment = &depth_attachment_ref,
		};

		vk::SubpassDependency dependency{
			.srcSubpass = vk::SubpassExternal,
			.dstSubpass = 0,
			.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
			.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
			.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
		};

		// Create RenderPass
		vk::RenderPassCreateInfo create_info{
			.attachmentCount = static_cast<uint32_t>(attachments.size()),
			.pAttachments = attachments.data(),
			.subpassCount = 1,
			.pSubpasses = &subpass_description,
			.dependencyCount = 1,
			.pDependencies = &dependency
		};
		vk_UniqueRenderPass = m_Device.GetDevice().createRenderPassUnique(create_info);
		vk_RenderPass = vk_UniqueRenderPass.get();
	}
}