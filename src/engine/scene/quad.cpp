#include "quad.h"

namespace VK_Renderer
{

	Quad::Quad() {
		Clear();

		m_Positions = {
			-1, 0, 1,
			1, 0, 1,
			-1, 0, -1,
			1, 0, -1
		};

		m_UVs = {
			0, 0,
			1, 0,
			0, 1,
			1, 1
		};

		m_Normals = {
			0, 1, 0
		};

		Triangle tri_0;
		tri_0.pId = glm::ivec3(0, 1, 2);
		tri_0.nId = glm::ivec3(0, 0, 0);
		tri_0.uvId = glm::ivec3(0, 1, 2);
		tri_0.materialId = -1;
		m_Triangles.push_back(std::move(tri_0));

		Triangle tri_1;
		tri_1.pId = glm::ivec3(1, 3, 2);
		tri_1.nId = glm::ivec3(0, 0, 0);
		tri_1.uvId = glm::ivec3(1, 3, 2);
		tri_1.materialId = -1;
		m_Triangles.push_back(std::move(tri_1));
	}
};