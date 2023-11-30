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

	void Scene::AddMesh(std::string const& file, 
						std::string const& name, 
						Transformation const& trans)
	{
		m_MeshFiles.emplace_back(file);

		m_MeshProxies.push_back(MeshProxy{
			.id = static_cast<uint32_t>(m_MeshFiles.size() - 1),
			.name = name,
			.transform = trans
		});

		glm::mat4 model = trans.GetTransformation();

		m_ModelMatries.push_back(ModelMatrix{
			.model =  model,
			.invModel = glm::transpose(glm::inverse(model))
		});
	}
	
	void Scene::ComputeRenderData(ComputeRenderDataInfo const& info)
	{
		ComputeMeshlet(info.MeshletMaxPrimCount, info.MeshletMaxVertexCount);
		ComputeAtlasTexture();
	}

	void Scene::UpdateModelMatrix(uint32_t const& id)
	{
		MeshProxy const& mesh = m_MeshProxies[id];
		glm::mat4 model = mesh.transform.GetTransformation();
		m_ModelMatries[mesh.id].model = model;
		m_ModelMatries[mesh.id].invModel = glm::transpose(glm::inverse(model));
	}

	void Scene::ComputeMeshlet(uint16_t const& maxPrimitiveCount, uint16_t const& maxVertexCount)
	{
		m_Meshlets.reset();
		m_Meshlets = mkU<Meshlets>(maxPrimitiveCount, maxVertexCount);

		uint32_t matertial_count = 0;
		for (size_t i = 0; i < m_MeshFiles.size(); ++i)
		{
			uPtr<Mesh> mesh = mkU<Mesh>(m_MeshFiles[i]);
			m_Meshlets->Append(*mesh, i);

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
