#pragma once

#include <vulkan/vulkan.hpp>

namespace VK_Renderer
{
	class VK_Instance;
	class VK_Device;
	class VK_Swapchain;

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

	protected:
		DeclarePtrWithGetFunc(protected, uPtr, VK_Instance, m, Instance);
		DeclarePtrWithGetFunc(protected, uPtr, VK_Device, m, Device);
		DeclarePtrWithGetFunc(protected, uPtr, VK_Swapchain, m, Swapchain);
	};
}