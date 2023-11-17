#include "testLayer.h"

using namespace MyCore;

class Sandbox : public Application
{
public:
	Sandbox()
	{
		PushLayer(mkU<RenderLayer>("TestLayer"));
	}

	~Sandbox()
	{

	}
};

uPtr<Application> MyCore::CreateApplication()
{
	return mkU<Sandbox>();
}