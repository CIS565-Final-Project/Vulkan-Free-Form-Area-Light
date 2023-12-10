#pragma once

#include "mesh.h"
#include "meshlet.h"
#include "material.h"
#include "atlasTexture.h"
#include "transformation.h"

namespace VK_Renderer
{
	struct ComputeRenderDataInfo
	{
		uint16_t MeshletMaxPrimCount;
		uint16_t MeshletMaxVertexCount;
	};

	struct MeshProxy
	{
		uint32_t id;
		std::string name;

		Transformation transform;
	};

	struct ModelMatrix
	{
		glm::mat4 model{ glm::mat4(1) };
		glm::mat4 invModel{ glm::mat4(1) };
	};

	class Scene
	{
	public:
		Scene();
		~Scene();

		void Free();

		void AddMesh(const std::string& file, 
					std::string const& name, 
					Transformation const& trans = {});

		void ComputeRenderData(ComputeRenderDataInfo const& info);
		void FreeRenderData();

		void UpdateModelMatrix(uint32_t const& id);

	protected:
		void ComputeMeshlet(uint16_t const& maxPrimitiveCount, uint16_t const& maxVertexCount);
		void ComputeAtlasTexture();

	protected:
		std::vector<std::string> m_MeshFiles;
		std::vector<MaterialInfo> m_MaterialInfos;

		DeclareWithGetFunc(protected, std::vector<MeshProxy>, m, MeshProxies);
		DeclareWithGetFunc(protected, std::vector<ModelMatrix>, m, ModelMatries);

		DeclarePtrWithGetFunc(protected, uPtr, Meshlets, m, Meshlets, const);
		DeclarePtrWithGetFunc(protected, uPtr, AtlasTexture2D, m, AtlasTex2D, const);
	};
};