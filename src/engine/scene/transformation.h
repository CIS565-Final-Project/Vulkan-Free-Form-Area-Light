#pragma once

#include <glm.hpp>
#include <gtx/quaternion.hpp>
#include <gtc/matrix_transform.hpp>

namespace VK_Renderer
{
	class Transformation
	{
	public:
		inline void Translate(glm::vec3 const& translate) {
			position += translate;
		}
		inline void Rotate(float const& angle, glm::vec3 const& axis)
		{
			rotation = glm::rotate(rotation, angle, axis);
		}
		inline void RotateAround(glm::vec3 const& point, glm::vec3 const& eulerAngle)
		{
			rotation = glm::quat(eulerAngle) * rotation; 

			glm::vec3 l = position - point;
			l = glm::quat(eulerAngle) * glm::vec4(l, 1.f);
			position = point + l;
		}
		inline glm::mat4 GetTransformation() const
		{
			glm::mat4 const R = glm::toMat4(rotation);
			glm::mat4 const T = glm::translate(glm::mat4(1.f), position);
			glm::mat4 const S = glm::scale(glm::mat4(1.f), scale);

			return T * R * S;
		}
	public:
		glm::quat rotation;
		glm::vec3 position{ 0, 0, 0 };
		glm::vec3 scale{ 1, 1, 1 };
	};
}