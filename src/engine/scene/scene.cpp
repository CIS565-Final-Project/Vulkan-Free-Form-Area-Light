#include "scene.h"

namespace VK_Renderer
{
	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
		Free();
	}

	void Scene::Free()
	{
		m_MeshFiles.clear();
		m_MaterialInfos.clear();
		m_Meshlets.reset();
		m_AtlasTex2D.reset();
	}

	void Scene::AddMesh(const std::string& file)
	{
		m_MeshFiles.emplace_back(file);
	}
	
	void Scene::ComputeRenderData(ComputeRenderDataInfo const& info)
	{
		ComputeMeshlet(info.MeshletMaxPrimCount, info.MeshletMaxVertexCount);
		ComputeAtlasTexture();
	}

	void Scene::ComputeMeshlet(uint16_t const& maxPrimitiveCount, uint16_t const& maxVertexCount)
	{
		m_Meshlets.reset();
		m_Meshlets = mkU<Meshlets>(maxPrimitiveCount, maxVertexCount);

		uint32_t matertial_count = 0;
		for (auto const& file : m_MeshFiles)
		{
			uPtr<Mesh> mesh = mkU<Mesh>(file);
			m_Meshlets->Append(*mesh);

			for (auto const& mat_info : mesh->m_MaterialInfos)
			{
				m_MaterialInfos.push_back(mat_info);
			}
		}
	}
	void Scene::ComputeAtlasTexture()
	{
		// Load textures
		std::vector<Material> materails;

		for (auto const& info : m_MaterialInfos)
		{
			materails.push_back(Material(info));
		}

		// compute atlas texture
		m_AtlasTex2D.reset();
		m_AtlasTex2D = mkU<AtlasTexture2D>(materails);
		
		// Recompute uvs
		for (Vertex& v : m_Meshlets->GetVertices())
		{
			TextureBlock2D const& atlas = m_AtlasTex2D->GetFinishedAtlas()[v.materialId.x];
			glm::vec2 start = static_cast<glm::vec2>(atlas.start) / static_cast<glm::vec2>(m_AtlasTex2D->GetResolution());
			glm::vec2 end = static_cast<glm::vec2>(atlas.start + glm::ivec2(atlas.width, atlas.height)) / static_cast<glm::vec2>(m_AtlasTex2D->GetResolution());

			glm::vec2 uv = v.uv;
			uv = glm::mix(start, end, uv);
			v.uv.x = uv.x;
			v.uv.y = uv.y;
		}
	}
}
