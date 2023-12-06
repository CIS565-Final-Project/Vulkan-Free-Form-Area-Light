#include "testLayer.h"

using namespace MyCore;

class Sandbox : public Application
{
public:
	Sandbox()
	{
		PushLayer(mkU<RenderLayer>("TestLayer_2"));
	}

	~Sandbox()
	{

	}
};

uPtr<Application> MyCore::CreateApplication()
{
	return mkU<Sandbox>();
}