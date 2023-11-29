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
		  m_MaxVertexCount(maxVertexCount)
	{}

	void Meshlets::Append(Mesh const& mesh)
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

						m_Vertices.push_back(Vertex{
							.position = {mesh.m_Positions[3 * vertex.x],
										 mesh.m_Positions[3 * vertex.x + 1],
										 mesh.m_Positions[3 * vertex.x + 2], 
										 1.f},
							.normal = {mesh.m_Normals[3 * vertex.y],
									   mesh.m_Normals[3 * vertex.y + 1],
									   mesh.m_Normals[3 * vertex.y + 2], 
									   0.f},
							.uv = {mesh.m_UVs[2 * vertex.z],
								   mesh.m_UVs[2 * vertex.z + 1], 0.f, 0.f},
							.materialId = {vertex.w + static_cast<int>(m_Materials.size()), 0, 0, 0}
						});
					}
					else 
					{
						triangles[i].back()[v] = (*it).second;
					}
				}
			}
		}

		// TODO: Step 1.2 - Clustering Vertices and Triangles

		// Step 2 - Assemble meshlets
		MeshletDescription meshlet{.vertexBegin = static_cast<uint32_t>(m_VertexIndices.size()),
									.primBegin  = static_cast<uint32_t>(m_PrimitiveIndices.size()), };
		std::unordered_map<uint32_t, uint8_t> meshlet_vertices;

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
					m_MeshletInfos.push_back(meshlet);
					meshlet.Reset();
					meshlet.primBegin = m_PrimitiveIndices.size();
					meshlet.vertexBegin = m_VertexIndices.size();
					
					meshlet_vertices.clear();
				}
			}
		}

		if (meshlet.primCount > 0) m_MeshletInfos.push_back(meshlet);

		// Load textures
		m_Materials.reserve(m_Materials.size() + mesh.GetMaterialCounts());
		for (auto const& mat_info : mesh.m_MaterialInfos)
		{
			m_Materials.emplace_back(mat_info);

			glm::ivec3 const& dim = m_Materials.back().GetAlbedoTex().GetResolution();
			m_TextureAtlas.width += dim.x;
			m_TextureAtlas.height += dim.y;
		}

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

	void Meshlets::CreateMaterialData()
	{
		std::set<TextureAtlas> avaliable_atlas;
		std::vector<TextureAtlas> finished_atlas;
		std::vector<TextureAtlas> ordered_atlas;

		avaliable_atlas.insert(m_TextureAtlas);

		struct MaterialProxy
		{
			uint32_t id;
			uint32_t size;
			bool operator <(MaterialProxy const& other) const
			{
				return size > other.size; // in decending order
			}
		};

		std::vector<MaterialProxy> sorted_materials;
		
		for (size_t i = 0; i < m_Materials.size(); ++i)
		{
			glm::ivec3 const& dim = m_Materials[i].GetAlbedoTex().GetResolution();
			sorted_materials.emplace_back(i, dim.x * dim.y);
		}
		std::sort(sorted_materials.begin(), sorted_materials.end());

		finished_atlas.resize(m_Materials.size());

		for (auto const& material_proxy : sorted_materials)
		{
			glm::ivec3 const& dim = m_Materials[material_proxy.id].GetAlbedoTex().GetResolution();
			for (TextureAtlas const& atlas : avaliable_atlas)
			{
				if (atlas.width >= dim.x && atlas.height >= dim.y)
				{
					// Step 1. Add a new atlas finished list
					finished_atlas[material_proxy.id] = TextureAtlas{
						.start = atlas.start,
						.width = static_cast<unsigned int>(dim.x),
						.height = static_cast<unsigned int>(dim.y),
					};
					// compute some necessary data
					glm::ivec2 end = atlas.start + glm::ivec2{dim.x, dim.y};

					unsigned int w = atlas.width - dim.x;
					unsigned int h = atlas.height - dim.y;

					// Step 2. Add splited atlas into aviable set
					if (w > 0)
					{
						avaliable_atlas.insert(TextureAtlas{
							.start = atlas.start + glm::ivec2{0, dim.y},
							.width = w,
							.height = static_cast<unsigned int>(dim.y),
						});
					}
					if (h > 0)
					{
						avaliable_atlas.insert(TextureAtlas{
							.start = atlas.start + glm::ivec2{dim.x, 0},
							.width = static_cast<unsigned int>(dim.x),
							.height = h,
						});
					}
					if (w > 0 && h > 0)
					{
						avaliable_atlas.insert(TextureAtlas{
							.start = end,
							.width = w,
							.height = h,
						});
					}

					// Step 3. update max extent
					m_Extent = glm::max(m_Extent, end);

					// Step 4. remove current atlas from aviable set
					avaliable_atlas.erase(atlas);

					break;
				}
			}
		}

		// copy texture data
		m_TextureData.resize(m_Extent.x* m_Extent.y * 4);
		for (size_t i = 0; i < finished_atlas.size(); ++i)
		{
			TextureAtlas const& atlas = finished_atlas[i];
			Material const& material = m_Materials[i];

			uint32_t start = (atlas.start.y * (m_Extent.x) + atlas.start.x) * 4;
			unsigned char const* data = reinterpret_cast<unsigned char*>(material.GetAlbedoTex().GetRawData());

			uint32_t size = atlas.width * 4 * sizeof(unsigned char);
			for (uint32_t h = 0; h < atlas.height; ++h)
			{
				uint32_t copy_start = start + h * m_Extent.x * 4;
				std::memcpy(m_TextureData.data() + copy_start, data + h * size, size);
			}
		}

		// recompute uv
		for (Vertex& v : m_Vertices)
		{
			TextureAtlas const& atlas = finished_atlas[v.materialId.x];
			glm::vec2 start = static_cast<glm::vec2>(atlas.start) / static_cast<glm::vec2>(m_Extent);
			glm::vec2 end = static_cast<glm::vec2>(atlas.start + glm::ivec2(atlas.width, atlas.height)) / static_cast<glm::vec2>(m_Extent);

			glm::vec2 uv = v.uv;
			uv = glm::mix(start, end, uv);
			v.uv.x = uv.x;
			v.uv.y = uv.y;
		}
	}
}