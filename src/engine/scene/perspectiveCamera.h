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

		std::array<glm::vec4, 6> GetPlanes() const;

	protected:
		glm::vec3 NDCtoCamera(glm::vec2 const& ndc) const;

		static glm::vec3 GetNormal(glm::vec3 const& v0, 
									glm::vec3 const& v1, 
									glm::vec3 const& v2);
		static float Distance2Plane(glm::vec3 const& o, 
								glm::vec3 const& p, 
								glm::vec3 const& normal);

	public:
		DeclareWithGetFunc(public, glm::mat4, m, ProjMatrix, const);
		DeclareWithGetFunc(public, glm::mat4, m, ViewMatrix, const);

	public:
		DeclareWithGetFunc(public, Transformation, m, Transform, const);

		float alpha{ 1.f };

		glm::ivec2 resolution;
		float fovy{ 45.f };
		float far{ 100.f };
		float near{ .1f };
	};
}