#include "mesh.h"

#include "tiny_obj_loader.h"
#include <iostream>

namespace VK_Renderer
{
	Mesh::Mesh(const std::string& file)
	{
		LoadMeshFromFile(file);
	}

	void Mesh::LoadMeshFromFile(const std::string& file)
	{
		Clear();

		tinyobj::ObjReaderConfig reader_config;
		reader_config.triangulate = true;

		tinyobj::ObjReader reader;

		if (!reader.ParseFromFile(file, reader_config)) {
			if (!reader.Error().empty()) {
				std::cerr << "TinyObjReader: " << reader.Error();
			}
			exit(1);
		}

		if (!reader.Warning().empty()) {
			std::cout << "TinyObjReader: " << reader.Warning();
		}

		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();
		auto& materials = reader.GetMaterials();
		
		m_MaterialCounts = materials.size();

		m_Positions.resize(attrib.vertices.size());
		m_Normals.resize(attrib.normals.size());
		m_UVs.resize(attrib.texcoords.size());

		std::memcpy(m_Positions.data(), attrib.vertices.data(), attrib.vertices.size() * sizeof(tinyobj::real_t));
		std::memcpy(m_Normals.data(), attrib.normals.data(), attrib.normals.size() * sizeof(tinyobj::real_t));
		std::memcpy(m_UVs.data(), attrib.texcoords.data(), attrib.texcoords.size() * sizeof(tinyobj::real_t));

		m_Triangles.resize(shapes.size());

		for (size_t i = 0; i < shapes.size(); ++i)
		{
			int index_offset = 0;
			m_Triangles[i].resize(shapes[i].mesh.num_face_vertices.size());
			m_TriangleCounts += shapes[i].mesh.num_face_vertices.size();
			for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++)
			{
				int fv = shapes[i].mesh.num_face_vertices[f];

				// Loop over vertices in the face.
				for (size_t v = 0; v < fv; v++) 
				{
					// access to vertex
					tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
					m_Triangles[i][f].pId[v] = idx.vertex_index;
					m_Triangles[i][f].nId[v] = idx.normal_index;
					m_Triangles[i][f].uvId[v] = idx.texcoord_index;
				}
				m_Triangles[i][f].materialId = shapes[i].mesh.material_ids[f];

				index_offset += fv;
			}
		}
	}
}