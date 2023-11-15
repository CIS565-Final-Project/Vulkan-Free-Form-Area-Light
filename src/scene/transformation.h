#pragma once

#include <glm.hpp>
#include <gtx/quaternion.hpp>

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

	public:
		glm::vec3 position;
		glm::quat rotation;
	};
}