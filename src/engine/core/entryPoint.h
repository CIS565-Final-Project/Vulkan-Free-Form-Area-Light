#pragma once
#include "pch.h"
#include "application.h"

using namespace MyCore;

void main()
{
	uPtr<Application> application = CreateApplication();
	application->Run();
	application.reset();
}