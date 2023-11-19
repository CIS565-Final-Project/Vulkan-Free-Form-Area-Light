#pragma once

#include <vulkan/vulkan.hpp>

#include "instance.h"
#include "swapchain.h"
#include "device.h"
#include "texture.h"
#include "renderPass.h"
#include "commandbuffer.h"
#include "commandPool.h"

namespace VK_Renderer
{
	class VK_RenderEngine
	{
	public:
		typedef std::function<bool(VkInstance, VkSurfaceKHR*)> CreateSurfaceFN;

	public:
		VK_RenderEngine();
		VK_RenderEngine(std::vector<char const*>const & instanceExtensions,
						CreateSurfaceFN createSurfaceFunc, 
						std::vector<char const*>const& deviceExtensions,
						vk::PhysicalDeviceFeatures2 phyDevFeature2,
						int const& width, 
						int const& height)
			: VK_RenderEngine()
		{
			Init(instanceExtensions, createSurfaceFunc, deviceExtensions, phyDevFeature2, width, height);
		}

		~VK_RenderEngine() { Reset(); }

		void Init(std::vector<char const*>const& instanceExtensions,
					CreateSurfaceFN createSurfaceFunc,
					std::vector<char const*>const& deviceExtensions,
					vk::PhysicalDeviceFeatures2 phyDevFeature2,
					int const& width,
					int const& height);
		void Reset();

		void WaitIdle() const;

		void RecordCommandBuffer();

		inline void PushCommand(vk::CommandBuffer const& cmd) { m_Commands.push_back(cmd); }
		inline void AddRenderFinishSemasphore(vk::Semaphore const& semaphore) { m_RenderFinishSemaphores.push_back(semaphore); }

		void OnRender(double const& deltaTime);

	protected:
		DeclarePtrWithGetFunc(protected, uPtr, VK_Instance, m, Instance);
		DeclarePtrWithGetFunc(protected, uPtr, VK_Device, m, Device);
		DeclarePtrWithGetFunc(protected, uPtr, VK_Swapchain, m, Swapchain);
		DeclarePtrWithGetFunc(protected, uPtr, VK_RenderPass, m, RenderPass);

		DeclarePtrWithGetFunc(protected, uPtr, VK_Texture2D, m, DepthTexture);
		DeclarePtrWithGetFunc(protected, uPtr, VK_Texture2D, m, ColorTexture);

		DeclarePtrWithGetFunc(protected, uPtr, VK_CommandBuffer, m, CommandBuffer);

		DeclareWithSetFunc(protected, vk::ClearColorValue, vk, ClearColor);

		vk::UniqueSemaphore vk_UniqueRenderFinishedSemaphore;

		std::vector<vk::CommandBuffer> m_Commands;
		std::vector<vk::Semaphore> m_RenderFinishSemaphores;
	};
}