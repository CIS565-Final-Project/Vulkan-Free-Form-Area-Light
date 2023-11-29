#pragma once

#include "material.h"

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
		Mesh()
			: m_MaterialCounts(0), m_TriangleCounts(0)
		{}
		Mesh(const std::string& file);

		virtual ~Mesh();

		void Free();

		void LoadMeshFromFile(const std::string& file);

	public:
		DeclareWithGetFunc(protected, uint32_t, m, MaterialCounts, const);
		DeclareWithGetFunc(protected, uint32_t, m, TriangleCounts, const);

		std::vector<Float> m_Positions;
		std::vector<Float> m_Normals;
		std::vector<Float> m_UVs;

		std::vector<std::vector<Triangle>> m_Triangles;

		std::vector<MaterialInfo> m_MaterialInfos;
	};
};