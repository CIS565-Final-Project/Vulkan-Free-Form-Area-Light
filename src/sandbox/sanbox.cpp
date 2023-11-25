#include "testLayer.h"

using namespace MyCore;

class Sandbox : public Application
{
public:
	Sandbox()
	{
		PushLayer(mkU<RenderLayer>("TestLayer_2", "meshes/wahoo.obj", "images/wahoo.bmp"));
		//PushLayer(mkU<RenderLayer>("TestLayer_1", "meshes/plane.obj", "images/wall.jpg"));
	}

	~Sandbox()
	{

	}
};

uPtr<Application> MyCore::CreateApplication()
{
	return mkU<Sandbox>();
}