#pragma once

#include <vulkan/vulkan.hpp>

namespace VK_Renderer
{
	class VK_Device;

	class VK_RenderPass
	{
	public:
		VK_RenderPass(VK_Device const& device);
		virtual ~VK_RenderPass() {};

		virtual void Create(vk::Format const& swapchainImageFormat);

	protected:
		VK_Device const& m_Device;

		vk::UniqueRenderPass vk_UniqueRenderPass;

		DeclareWithGetFunc(protected, vk::RenderPass, vk, RenderPass, const);
	};
}