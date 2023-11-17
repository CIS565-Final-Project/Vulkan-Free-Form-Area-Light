#pragma once

#include <glm.hpp>

#include "transformation.h"

namespace VK_Renderer
{
	class PerspectiveCamera
	{
	public:
		void RecomputeProjView();
		inline glm::mat4 GetProjViewMatrix() const { return m_ProjMatrix * m_ViewMatrix; }

	public:
		DeclareWithGetFunc(public, glm::mat4, m, ProjMatrix, const);
		DeclareWithGetFunc(public, glm::mat4, m, ViewMatrix, const);

	public:
		DeclareWithGetFunc(public, Transformation, m, Transform, const);

		glm::ivec2 resolution;
		float fovy{ 45.f };
		float far{ 100.f };
		float near{ .1f };
	};
}