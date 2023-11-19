#include "application.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include "renderEngine/renderEngine.h"

namespace MyCore
{
	Application* Application::s_Instance = nullptr;

	Application::Application(std::string const& appConfigFile)
		:b_IsRunning(false)
	{
		assert(!s_Instance);
		s_Instance = this;
		// TODO: load app configuration from file

		// Create Window
		SDL_Init(SDL_INIT_VIDEO);

		SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

		SDL_Window* window = SDL_CreateWindow(
			"Vulkan Hello Triangle",
			200, 200, 680, 680, window_flags
		);
		m_Window = window;
		std::vector<char const*> extensions;

		unsigned int sdl_ext_count = 0;
		SDL_Vulkan_GetInstanceExtensions(window, &sdl_ext_count, NULL);

		extensions.resize(sdl_ext_count);
		SDL_Vulkan_GetInstanceExtensions(window, &sdl_ext_count, extensions.data());

		std::vector<const char*> deviceEXTs{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_EXT_MESH_SHADER_EXTENSION_NAME
		};
		vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceMeshShaderFeaturesEXT> ext_chain;
		
		int width, height;
		SDL_GetWindowSize(window, &width, &height);

		m_RenderEngine = mkU<VK_Renderer::VK_RenderEngine>(extensions,
															std::bind(SDL_Vulkan_CreateSurface, window, std::placeholders::_1, std::placeholders::_2),
															deviceEXTs,
															ext_chain.get<vk::PhysicalDeviceFeatures2>(),
															width, height);
	}
	
	Application::~Application()
	{
		m_RenderEngine->WaitIdle();

		for (auto& layer : m_Layers)
		{
			layer->OnDetech();
		}
		m_Layers.clear();
		m_RenderEngine.reset();

		SDL_Window* window = reinterpret_cast<SDL_Window*>(m_Window);
		SDL_DestroyWindowSurface(window);
		SDL_DestroyWindow(window);
	}

	void Application::Run()
	{
		m_RenderEngine->RecordCommandBuffer();

		b_IsRunning = true;
		SDL_Event e;
		while (b_IsRunning)
		{
			while (SDL_PollEvent(&e) != 0) 
			{
				// compute delta time
				double delta_t = 0.f;

				if (e.type == SDL_QUIT) b_IsRunning = false;
				// OnEvent
				for (auto& layer : m_Layers)
				{
					layer->OnEvent(e);
				}

				// OnUpdate
				for (auto& layer : m_Layers)
				{
					layer->OnUpdate(delta_t);
				}

				// OnRender
				for (auto& layer : m_Layers)
				{
					layer->OnRender(delta_t);
				}
				m_RenderEngine->OnRender(delta_t);

				// TODO: OnImGui
				for (auto& layer : m_Layers)
				{
					layer->OnImGui(delta_t);
				}
			}
		}
	}
}