#pragma once

#include "layer.h"

namespace VK_Renderer
{
	class VK_RenderEngine;
};

namespace MyCore
{
	class Layer;

	class Application
	{
	public:
		Application(std::string const& appConfigFile = "");
		virtual ~Application();

		inline void PushLayer(uPtr<Layer>&& layer) 
		{ 
			layer->OnAttach();
			m_Layers.emplace_back(std::move(layer)); 
		}

		void Run();

		inline static Application* GetInstance() { return s_Instance; }

	private:
		static Application* s_Instance;

	protected:
		void* m_Window;

		DeclarePtrWithGetFunc(protected, uPtr, VK_Renderer::VK_RenderEngine, m, RenderEngine);
		DeclareWithGetFunc(protected, bool, b, IsRunning, const);

		std::vector<uPtr<Layer>> m_Layers;
	};

	uPtr<Application> CreateApplication();
}