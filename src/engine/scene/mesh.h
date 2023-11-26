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
			m_MaterialCounts = 0;
			m_TriangleCounts = 0;
			m_Positions.clear();
			m_Normals.clear();
			m_UVs.clear();
			m_Triangles.clear();
		}
		void LoadMeshFromFile(const std::string& file);

	public:
		DeclareWithGetFunc(protected, uint32_t, m, MaterialCounts, const);
		DeclareWithGetFunc(protected, uint32_t, m, TriangleCounts, const);

		std::vector<Float> m_Positions;
		std::vector<Float> m_Normals;
		std::vector<Float> m_UVs;

		std::vector<std::vector<Triangle>> m_Triangles;
	};
};