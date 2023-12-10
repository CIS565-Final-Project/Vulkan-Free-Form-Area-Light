#include "scene.h"
#include <iostream>

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
#ifndef NDEBUG
		std::cout << "Start Loading models......" << std::endl;
#endif
		ComputeMeshlet(info.MeshletMaxPrimCount, info.MeshletMaxVertexCount);
#ifndef NDEBUG
		std::cout << "Loading Scene Success!\n" << std::endl;
		std::cout << "Start Loading Textures......" << std::endl;
#endif
		ComputeAtlasTexture();
#ifndef NDEBUG
		std::cout << "Loading textrues Success!" << std::endl;
#endif
	}
	
	void Scene::FreeRenderData()
	{
		m_Meshlets->FreeData();
		m_AtlasTex2D.reset();
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

#ifndef NDEBUG
		std::string temp = R"(Loading Mesh: {}
Model details:
Vertex count: {}
Triangles counts: {}
Materials counts: {}
Meshlet count: {}
Original indices buffer size: {} bytes
Meshlet size: {} bytes
Improved: {}%
			)";

		struct MeshletSum
		{
			uint32_t v_count{ 0 };
			uint32_t t_count{ 0 };
			uint32_t m_count{ 0 };
			uint32_t meshlet_count{ 0 };
			uint32_t idx_size{ 0 };
			uint32_t meshlet_size{ 0 };
			MeshletSum()
			{
			}
			MeshletSum(Meshlets const& meshlets)
			{
				v_count = meshlets.GetVertices().size();
				t_count = meshlets.GetTriangleCount();
				m_count = meshlets.GetMaterialOffset();
				meshlet_count = meshlets.GetMeshletInfos().size();
				idx_size = meshlets.GetTriangleCount() * sizeof(glm::ivec3);
				meshlet_size = sizeof(MeshletDescription) * meshlets.GetMeshletInfos().size() +
								sizeof(uint8_t) * meshlets.GetPrimitiveIndices().size() +
								sizeof(uint32_t) * meshlets.GetVertexIndices().size();
			}

			MeshletSum& operator+(MeshletSum const& other)
			{
				v_count += other.v_count;
				t_count += other.t_count;
				m_count += other.m_count;
				meshlet_count += other.meshlet_count;
				idx_size += other.idx_size;
				meshlet_size += other.meshlet_size;

				return *this;
			}

			MeshletSum& operator-(MeshletSum const& other)
			{
				v_count -= other.v_count;
				t_count -= other.t_count;
				m_count -= other.m_count;
				meshlet_count -= other.meshlet_count;
				idx_size -= other.idx_size;
				meshlet_size -= other.meshlet_size;

				return *this;
			}

			MeshletSum& operator=(MeshletSum const& other)
			{
				v_count = other.v_count;
				t_count = other.t_count;
				m_count = other.m_count;
				meshlet_count = other.meshlet_count;
				idx_size = other.idx_size;
				meshlet_size = other.meshlet_size;

				return *this;
			}
		};

		MeshletSum sum = {};
#endif
		uint32_t matertial_count = 0;
		for (size_t i = 0; i < m_MeshFiles.size(); ++i)
		{
			uPtr<Mesh> mesh = mkU<Mesh>(m_MeshFiles[i]);
			m_Meshlets->Append(*mesh, i);

			for (auto const& mat_info : mesh->m_MaterialInfos)
			{
				m_MaterialInfos.push_back(mat_info);
			}
#ifndef NDEBUG
			MeshletSum new_sum(*m_Meshlets);
			MeshletSum increased_sum = (new_sum - sum);
			std::cout << std::vformat(temp, std::make_format_args(
				m_MeshFiles[i],
				increased_sum.v_count,
				increased_sum.t_count,
				increased_sum.m_count,
				increased_sum.meshlet_count,
				increased_sum.idx_size,
				increased_sum.meshlet_size,
				(1.0 - static_cast<double>(increased_sum.meshlet_size) / static_cast<double>(increased_sum.idx_size)) * 100.0
			)) << std::endl;

			sum = sum + increased_sum;
#endif
		}
#ifndef NDEBUG
		temp = R"(Summary:
Vertex count: {}
Triangles counts: {}
Materials counts: {}
Meshlet count: {}
Original indices buffer size: {} bytes
Meshlet size: {} bytes
Improved: {}%
			)";
		std::cout << std::vformat(temp, std::make_format_args(
			sum.v_count,
			sum.t_count,
			sum.m_count,
			sum.meshlet_count,
			sum.idx_size,
			sum.meshlet_size,
			(1.0 - static_cast<double>(sum.meshlet_size) / static_cast<double>(sum.idx_size)) * 100.0
		)) << std::endl;
#endif
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
			if (v.materialId.x < 0) continue;

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
