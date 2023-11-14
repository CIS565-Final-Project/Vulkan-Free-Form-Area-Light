#pragma once

#include <glm.hpp>

#include "transformable.h"

namespace VK_Renderer
{
	class PerspectiveCamera : public Transformable
	{

	public:
		void RecomputeProjView();
		inline glm::mat4 GetProjViewMatrix() const { return m_ProjMatrix * m_ViewMatrix; }

	protected:
		DeclareWithGetFunc(protected, glm::mat4, m, ProjMatrix, const);
		DeclareWithGetFunc(protected, glm::mat4, m, ViewMatrix, const);

	public:
		glm::ivec2 resolution;
		float fovy{ 45.f };
		float far{ 100.f };
		float near{ .1f };
	};
}