#include "meshlet.h"

namespace std {
	template<>
	struct hash<glm::ivec4> {
		size_t operator()(const glm::ivec4& v) const {
			// Use a combination of hashes for each fielv
			return hash<int>()(v.x) ^ 
					((hash<int>()(v.y) << 1)) ^ 
					((hash<int>()(v.z) << 2)) ^ 
					((hash<int>()(v.w) << 3));
		}
	};
}

namespace VK_Renderer
{
	Meshlets::Meshlets(uint16_t const& maxPrimitiveCount, uint16_t const& maxVertexCount)
		: m_MaxPrimitiveCount(maxPrimitiveCount), 
		  m_MaxVertexCount(maxVertexCount),
		  m_MaterialOffset(0),
		  m_TriangleCount(0),
		  m_MeshletsCount(0)
	{}

	void Meshlets::Append(Mesh const& mesh, uint32_t const& modelId)
	{
		if (mesh.GetTriangleCounts() < 1) return;
		// Step 1 - Clustering Vertices and Triangles
		
		// Step 1.1 - Remove dulplicate vertices
		// store the idx of tree vertices after removing dulplicate vertices
		std::vector<std::vector<glm::ivec3>> triangles;
		// store the idx of position, normal, uv, material_id
		std::vector<glm::ivec4> unique_vertices;

		triangles.resize(mesh.m_Triangles.size());

		for (size_t i = 0; i < mesh.m_Triangles.size(); ++i)
		{
			std::unordered_map<glm::ivec4, uint32_t> unordered_map;
			for (auto const& triangle : mesh.m_Triangles[i])
			{
				triangles[i].emplace_back(0, 0, 0);
				for (int v = 0; v < 3; ++v)
				{
					glm::ivec4 vertex(triangle.pId[v], triangle.nId[v], triangle.uvId[v], triangle.materialId);
					
					auto it = unordered_map.find(vertex);

					if (it == unordered_map.end())
					{
						unique_vertices.push_back(vertex);
						unordered_map[vertex] = m_Vertices.size();
						triangles[i].back()[v] = m_Vertices.size();

						glm::vec2 uv = vertex.z > 0 ? glm::vec2( mesh.m_UVs[2 * vertex.z],
							mesh.m_UVs[2 * vertex.z + 1]) : glm::vec2(0.f, 0.f);

						m_Vertices.push_back(Vertex{
							.position = {mesh.m_Positions[3 * vertex.x],
										 mesh.m_Positions[3 * vertex.x + 1],
										 mesh.m_Positions[3 * vertex.x + 2], 
										 1.f},
							.normal = {mesh.m_Normals[3 * vertex.y],
									   mesh.m_Normals[3 * vertex.y + 1],
									   mesh.m_Normals[3 * vertex.y + 2], 
									   0.f},
							.uv = {uv.x, uv.y, 0.f, 0.f},
							.materialId = {vertex.w + static_cast<int>(m_MaterialOffset), 0, 0, 0}
						});
					}
					else 
					{
						triangles[i].back()[v] = (*it).second;
					}
				}
			}
		}
		m_MaterialOffset += mesh.GetMaterialCounts();
		for (auto const& tri : mesh.m_Triangles)
		{
			m_TriangleCount += tri.size();
		}
		// TODO: Step 1.2 - Clustering Vertices and Triangles

		// Step 2 - Assemble meshlets
		MeshletDescription meshlet{ .modelId = modelId,
									.vertexBegin = static_cast<uint32_t>(m_VertexIndices.size()),
									.primBegin  = static_cast<uint32_t>(m_PrimitiveIndices.size()), 
									};
		std::unordered_map<uint32_t, uint8_t> meshlet_vertices;
		std::vector<glm::vec3> vertices_in_meshlet;

		for (size_t i = 0; i < triangles.size(); ++i)
		{
			// sort triangles (pseudo clustering)
			//std::sort(tris.begin(), tris.end(), [](glm::ivec3 const& t1, glm::ivec3 const& t2) {
			//	return (t1.x != t2.x ? t1.x < t2.x :
			//		(t1.y != t2.y ? t1.y < t2.y :
			//			(t1.z < t2.z)));
			//});
			
			for (glm::ivec3 const& tri : triangles[i])
			{
				for (int v = 0; v < 3; ++v)
				{
					auto it = meshlet_vertices.find(tri[v]);
					if (it == meshlet_vertices.end())
					{
						// if vertex not yes in the meshlet
						// Add vertices
						m_VertexIndices.push_back(tri[v]);
						m_PrimitiveIndices.push_back(meshlet.vertexCount);
						meshlet_vertices[tri[v]] = meshlet.vertexCount;
						
						vertices_in_meshlet.push_back(m_Vertices[tri[v]].position);

						++meshlet.vertexCount;
					}
					else // if vertex already in the meshlet
					{
						m_PrimitiveIndices.push_back((*it).second);
					}
				}
				++meshlet.primCount;

				// Check whether to start a new meshlet
				if (meshlet.primCount >= m_MaxPrimitiveCount ||
					meshlet.vertexCount >= m_MaxVertexCount)
				{
					ComputeBoundingSphere(meshlet, vertices_in_meshlet);

					m_MeshletInfos.push_back(meshlet);
					meshlet.Reset();
					meshlet.primBegin = m_PrimitiveIndices.size();
					meshlet.vertexBegin = m_VertexIndices.size();
					meshlet.modelId = modelId;
					meshlet.boudningSphere = glm::vec4(modelId);
					meshlet_vertices.clear();
					vertices_in_meshlet.clear();
				}
			}
		}

		if (meshlet.primCount > 0) {
			ComputeBoundingSphere(meshlet, vertices_in_meshlet);
			m_MeshletInfos.push_back(meshlet);
		}

		m_MeshletsCount = m_MeshletInfos.size();

		// Load textures
		//m_Materials.reserve(m_Materials.size() + mesh.GetMaterialCounts());
		//for (auto const& mat_info : mesh.m_MaterialInfos)
		//{
		//	m_Materials.emplace_back(mat_info);
		//
		//	glm::ivec3 const& dim = m_Materials.back().GetAlbedoTex().GetResolution();
		//}

/*
		std::vector<glm::ivec3> out_triangles;
		for (auto const& meshlet : m_MeshletInfos)
		{
			for (int i = 0; i < meshlet.primCount; ++i)
			{
				glm::ivec3 local_id{
					m_PrimitiveIndices[meshlet.primBegin + 3 * i],
					m_PrimitiveIndices[meshlet.primBegin + 3 * i + 1],
					m_PrimitiveIndices[meshlet.primBegin + 3 * i + 2] 
				};

				glm::ivec3 real_id{
					m_VertexIndices[meshlet.vertexBegin + static_cast<uint32_t>(local_id.x)],
					m_VertexIndices[meshlet.vertexBegin + static_cast<uint32_t>(local_id.y)],
					m_VertexIndices[meshlet.vertexBegin + static_cast<uint32_t>(local_id.z)]
				};

				out_triangles.push_back(real_id);
			}
		}

		printf("original: %d\n", sizeof(glm::ivec3) * out_triangles.size());
		printf("meshlets: %d\n", sizeof(MeshletDescription)* m_MeshletInfos.size() + 
								 sizeof(uint8_t) * m_PrimitiveIndices.size() + 
								 sizeof(uint32_t) * m_VertexIndices.size());
*/
	}

	void Meshlets::FreeData()
	{
		m_Vertices.clear();
		m_MeshletInfos.clear();
		m_PrimitiveIndices.clear();
		m_VertexIndices.clear();
	}

	void Meshlets::ComputeBoundingSphere(MeshletDescription& meshletDesc,
											std::vector<glm::vec3> const& vertices)
	{
		// Ritter's Bounding Sphere Algorithm
		glm::vec3 const& p0 = vertices[0];

		// find P1
		float max_dist = -1.f;
		glm::vec3 const* p1_ptr = nullptr;
		for (glm::vec3 const& p : vertices)
		{
			float dist = glm::distance(p0, p);
			if (dist > max_dist)
			{
				max_dist = dist;
				p1_ptr = &p;
			}
		}
		// find P2
		max_dist = -1.f;
		glm::vec3 const* p2_ptr = nullptr;
		for (glm::vec3 const& p : vertices)
		{
			float dist = glm::distance(p, *p1_ptr);
			if (dist > max_dist)
			{
				max_dist = dist;
				p2_ptr = &p;
			}
		}

		// compute center and radius
		
		glm::vec3 center = (*p1_ptr + *p2_ptr) / 2.f;
		float radius = max_dist / 2.f;

		for (glm::vec3 const& p : vertices)
		{
			float dist = glm::distance(p, center);
			if (dist > radius)
			{
				radius = (radius + dist) / 2.f;
				center = center * 0.5f + p * 0.5f;
			}
		}

		meshletDesc.boudningSphere = glm::vec4(center, radius);
	}
}