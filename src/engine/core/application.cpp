#include "application.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include "renderEngine/renderEngine.h"

#include "layers/imguiLayer.h"

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
			200, 200, 1000, 1000, window_flags
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

		// push build-in layers
		PushLayer(mkU<ImGuiLayer>("ImGuiLayer", window, *m_RenderEngine));
		m_ImGuiLayer = reinterpret_cast<ImGuiLayer*>(m_Layers.back().get());
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
				
				if (!m_ImGuiLayer->HandleEvents(e))
				{
					for (auto& layer : m_Layers)
					{
						if (layer->OnEvent(e)) break;
					}
				}

				m_ImGuiLayer->BeginFrame();
				for (auto& layer : m_Layers)
				{
					layer->OnImGui(delta_t);
				}
				m_ImGuiLayer->EndFrame();

				// OnUpdate
				for (auto& layer : m_Layers)
				{
					layer->OnUpdate(delta_t);
				}

				// OnRender
				m_RenderEngine->BeforeRender();

				//printf("current frame: %d\n", m_RenderEngine->GetSwapchain()->GetImageIdx());
				for (auto& layer : m_Layers)
				{
					layer->OnRender(delta_t);
				}
				m_RenderEngine->RecordCommandBuffer();
				m_RenderEngine->Render();
			}
		}
	}
}