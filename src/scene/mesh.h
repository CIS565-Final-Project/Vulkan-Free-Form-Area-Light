#pragma once

namespace VK_Renderer
{
	struct Triangle
	{
		glm::ivec3 pId;
		glm::ivec3 nId;
		glm::ivec3 uvId;
		int materialId;
	};

	class Mesh
	{
	public:
		Mesh() {}
		Mesh(const std::string& file);

		void Clear()
		{
			m_Positions.clear();
			m_Normals.clear();
			m_UVs.clear();
			m_Triangles.clear();
		}
		void LoadMeshFromFile(const std::string& file);

	public:
		std::vector<Float> m_Positions;
		std::vector<Float> m_Normals;
		std::vector<Float> m_UVs;

		std::vector<Triangle> m_Triangles;
	};
};