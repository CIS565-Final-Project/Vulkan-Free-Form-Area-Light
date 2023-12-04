#include "perspectiveCamera.h"

#include <gtc/matrix_transform.hpp>
#include <iostream>
namespace VK_Renderer
{
	void PerspectiveCamera::RecomputeProjView()
	{
		glm::mat3 R = glm::toMat3(m_Transform.rotation);
		glm::vec3 const& forward = -R[2];

		glm::vec3 up = glm::vec3(0.f, glm::abs(forward.x) < glm::epsilon<float>() ? glm::sign(R[1].y) : 1.f, 0.f);
		glm::vec3 right = glm::normalize(glm::cross(-forward, up));
		up = glm::cross(right, -R[2]);

		m_ViewMatrix = transpose(glm::mat4(
			glm::vec4(right, 0.f),
			glm::vec4(up, 0.f),
			glm::vec4(forward, 0.f),
			glm::vec4(0.f, 0.f, 0.f, 1.f)
		)) *
		glm::mat4(
			glm::vec4(1.f, 0.f, 0.f, 0.f),
			glm::vec4(0.f, 1.f, 0.f, 0.f),
			glm::vec4(0.f, 0.f, 1.f, 0.f),
			glm::vec4(-m_Transform.position, 1)
		);
		float const aspect = resolution.x / resolution.y;
		
		float const S = 1.f / glm::tan(fovy / 2.f);
		float const P = far / (far - near);
		float const Q = -P * near;

		m_ProjMatrix = glm::perspective(fovy, aspect, near, far);

		//m_ProjMatrix = {
		//	glm::vec4(S / aspect, 0.f, 0.f, 0.f),
		//	glm::vec4(0.f, S, 0.f, 0.f),
		//	glm::vec4(0.f, 0.f, P, Q),
		//	glm::vec4(0.f, 0.f, 1.f, 0.f),
		//};

		//m_ProjMatrix = glm::transpose(m_ProjMatrix);
	}
	std::array<glm::vec4, 6> PerspectiveCamera::GetPlanes() const
	{
		glm::mat3 R = glm::toMat3(m_Transform.rotation);
		glm::vec3 const& forward = -R[2];

		glm::vec3 up = glm::vec3(0.f, glm::abs(forward.x) < glm::epsilon<float>() ? glm::sign(R[1].y) : 1.f, 0.f);
		glm::vec3 right = glm::normalize(glm::cross(-forward, up));
		up = glm::cross(right, -R[2]);

		static std::array<glm::vec2, 4> const ndcs{
			glm::vec2{-1, -1}, {1, -1}, {1, 1}, {-1, 1}
		};
		std::array<std::array<glm::vec3, 4>, 2> frustum_p;
		for (int i = 0; i < 4; ++i)
		{
			glm::vec3 p_camera = NDCtoCamera(alpha * ndcs[i]);

			frustum_p[0][i] = m_Transform.position + near * (p_camera.x * right + p_camera.y * up - forward);
			frustum_p[1][i] = m_Transform.position + far * (p_camera.x * right + p_camera.y * up - forward);
		}
		//for (int i = 0; i < 2; ++i)
		//{
		//	for (int j = 0; j < 4; ++j)
		//	{
		//		printf("%f, %f, %f\n", frustum_p[i][j].x, frustum_p[i][j].y, frustum_p[i][j].z);
		//	}
		//}
		std::array<glm::vec3, 6> normals{
			GetNormal(frustum_p[0][0], frustum_p[0][2], frustum_p[0][1]), // near
			GetNormal(frustum_p[1][0], frustum_p[1][1], frustum_p[1][2]), // far
			GetNormal(frustum_p[0][2], frustum_p[0][3], frustum_p[1][3]), // top
			GetNormal(frustum_p[0][0], frustum_p[0][1], frustum_p[1][1]), // bottom
			GetNormal(frustum_p[0][0], frustum_p[1][0], frustum_p[0][3]), // left
			GetNormal(frustum_p[0][1], frustum_p[0][2], frustum_p[1][1]), // right
		};
		return {
			glm::vec4(normals[0], Distance2Plane({0.f, 0.f, 0.f}, frustum_p[0][0], normals[0])), // near
			glm::vec4(normals[1], Distance2Plane({0.f, 0.f, 0.f}, frustum_p[1][2], normals[1])), // far
			glm::vec4(normals[3], Distance2Plane({0.f, 0.f, 0.f}, frustum_p[0][2], normals[3])), // top
			glm::vec4(normals[2], Distance2Plane({0.f, 0.f, 0.f}, frustum_p[0][0], normals[2])), // bottom
			glm::vec4(normals[4], Distance2Plane({0.f, 0.f, 0.f}, frustum_p[0][0], normals[4])), // left
			glm::vec4(normals[5], Distance2Plane({0.f, 0.f, 0.f}, frustum_p[0][1], normals[5]))  // right
		};
	}
	glm::vec3 PerspectiveCamera::NDCtoCamera(glm::vec2 const& ndc) const
	{
		float aspect = static_cast<float>(resolution.x) / static_cast<float>(resolution.y);
		float radian = glm::radians(fovy * 0.5f);

		return {
			ndc.x* glm::tan(radian)* aspect,
			ndc.y* glm::tan(radian),
			1.f
		};
	}

	glm::vec3 PerspectiveCamera::GetNormal(glm::vec3 const& v0, glm::vec3 const& v1, glm::vec3 const& v2)
	{
		return glm::normalize(glm::cross(v1 - v0, v2 - v0));
	}

	float PerspectiveCamera::Distance2Plane(glm::vec3 const& o,
											glm::vec3 const& p, 
											glm::vec3 const& normal)
	{
		return glm::abs(glm::dot(p - o, normal));
	}
}