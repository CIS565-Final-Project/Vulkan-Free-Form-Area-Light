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

		inline void PushSecondaryCommandAll(vk::CommandBuffer const& cmd)
		{
			for (auto& secondary_cmd_list : m_SecondaryCommands)
			{
				secondary_cmd_list.push_back(cmd);
			}
		}

		inline void PushSecondaryCommand(vk::CommandBuffer const& cmd, uint32_t const& primaryIdx) 
		{
			m_SecondaryCommands[primaryIdx].push_back(cmd);
		}
		inline void AddRenderFinishSemasphore(vk::Semaphore const& semaphore) { m_RenderFinishSemaphores.push_back(semaphore); }

		void WaitForFence();

		void BeforeRender();
		void Render();

	protected:
		DeclarePtrWithGetFunc(protected, uPtr, VK_Instance, m, Instance, const);
		DeclarePtrWithGetFunc(protected, uPtr, VK_Device, m, Device, const);
		DeclarePtrWithGetFunc(protected, uPtr, VK_Swapchain, m, Swapchain, const);
		DeclarePtrWithGetFunc(protected, uPtr, VK_RenderPass, m, RenderPass, const);

		DeclarePtrWithGetFunc(protected, uPtr, VK_Texture2D, m, DepthTexture);
		DeclarePtrWithGetFunc(protected, uPtr, VK_Texture2D, m, ColorTexture);

		DeclarePtrWithGetFunc(protected, uPtr, VK_CommandBuffer, m, CommandBuffer);

		DeclareWithGetSetFunc(protected, vk::ClearColorValue, vk, ClearColor);

		vk::UniqueSemaphore vk_UniqueRenderFinishedSemaphore;

		std::vector<vk::CommandBuffer> m_PrimaryCommands;
		std::vector<std::vector<vk::CommandBuffer>> m_SecondaryCommands;
		std::vector<vk::Semaphore> m_RenderFinishSemaphores;

		std::vector<vk::UniqueFence> m_Fences;
	};
}