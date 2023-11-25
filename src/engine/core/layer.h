#pragma once

union SDL_Event;

namespace MyCore
{
	class Layer
	{
	public:
		Layer(std::string const& name);
		virtual ~Layer();

		virtual void OnAttach() = 0;
		virtual void OnDetech() = 0;

		virtual void OnUpdate(double const& deltaTime) {}
		virtual void OnRender(double const& deltaTime) {}
		virtual void OnImGui(double const& deltaTime) {}

		virtual bool OnEvent(SDL_Event const&) { return false; }

	protected:
		DeclareWithGetFunc(protected, std::string, m, Name, const);
	};
}