#include "perspectiveCamera.h"


namespace VK_Renderer
{
	void PerspectiveCamera::RecomputeProjView()
	{
		m_ViewMatrix = glm::toMat4(rotation) * 
		glm::mat4(
			glm::vec4(1.f, 0.f, 0.f, -position.x),
			glm::vec4(0.f, 1.f, 0.f, -position.y),
			glm::vec4(0.f, 0.f, 1.f, -position.z),
			glm::vec4(0, 0, 0, 1)
		);

		float const aspect = resolution.x / resolution.y;
		
		float const S = 1.f / glm::tan(fovy / 2.f);
		float const P = far / (far - near);
		float const Q = -P * near;

		m_ProjMatrix = {
			glm::vec4(S / aspect, 0.f, 0.f, 0.f),
			glm::vec4(0.f, S, 0.f, 0.f),
			glm::vec4(0.f, 0.f, P, Q),
			glm::vec4(0.f, 0.f, 1.f, 0.f),
		};
	}
}