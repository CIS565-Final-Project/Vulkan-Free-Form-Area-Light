#include "perspectiveCamera.h"

#include <gtc/matrix_transform.hpp>

namespace VK_Renderer
{
	void PerspectiveCamera::RecomputeProjView()
	{
		glm::mat3 R = glm::toMat3(m_Transform.rotation);
		
		m_ViewMatrix = transpose(glm::mat4(
			glm::vec4(-R[0], 0.f),
			glm::vec4( R[1], 0.f),
			glm::vec4(-R[2], 0.f),
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

		//m_ViewMatrix = glm::lookAt(m_Transform.position, { 0, 0, 0 }, {0, 1, 0});
		m_ProjMatrix = glm::perspective(fovy, aspect, near, far);

		//m_ProjMatrix = {
		//	glm::vec4(S / aspect, 0.f, 0.f, 0.f),
		//	glm::vec4(0.f, S, 0.f, 0.f),
		//	glm::vec4(0.f, 0.f, P, Q),
		//	glm::vec4(0.f, 0.f, 1.f, 0.f),
		//};
	}
}