#pragma once

#include "mesh.h"
#include "meshlet.h"
#include "material.h"
#include "atlasTexture.h"

namespace VK_Renderer
{
	struct ComputeRenderDataInfo
	{
		uint16_t MeshletMaxPrimCount;
		uint16_t MeshletMaxVertexCount;
	};

	class Scene
	{
	public:
		Scene();
		~Scene();

		void Free();

		void AddMesh(const std::string& file);

		void ComputeRenderData(ComputeRenderDataInfo const& info);
		
	protected:
		void ComputeMeshlet(uint16_t const& maxPrimitiveCount, uint16_t const& maxVertexCount);
		void ComputeAtlasTexture();

	protected:
		std::vector<std::string> m_MeshFiles;
		std::vector<MaterialInfo> m_MaterialInfos;

		DeclarePtrWithGetFunc(protected, uPtr, Meshlets, m, Meshlets, const);
		DeclarePtrWithGetFunc(protected, uPtr, AtlasTexture2D, m, AtlasTex2D, const);
	};
};