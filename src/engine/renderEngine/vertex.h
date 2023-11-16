#pragma once

namespace VK_Renderer
{
	struct Vertex
	{
		std::array<Float, 3> position;
		std::array<Float, 3> normal;
		std::array<Float, 2> uv;
	};
}