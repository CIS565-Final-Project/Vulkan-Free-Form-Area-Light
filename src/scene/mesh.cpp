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

		m_Positions.resize(attrib.vertices.size());
		m_Normals.resize(attrib.normals.size());
		m_UVs.resize(attrib.texcoords.size());

		std::memcpy(m_Positions.data(), attrib.vertices.data(), attrib.vertices.size() * sizeof(tinyobj::real_t));
		std::memcpy(m_Normals.data(), attrib.normals.data(), attrib.normals.size() * sizeof(tinyobj::real_t));
		std::memcpy(m_UVs.data(), attrib.texcoords.data(), attrib.texcoords.size() * sizeof(tinyobj::real_t));

		int index_offset = 0;
		for (const auto& shape : shapes)
		{
			for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) 
			{
				int fv = shape.mesh.num_face_vertices[f];

				// Loop over vertices in the face.
				Triangle triangle;
				for (size_t v = 0; v < fv; v++) 
				{
					
					// access to vertex
					tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
					triangle.pId[v] = 3 * idx.vertex_index;
					triangle.nId[v] = 3 * idx.normal_index;
					triangle.uvId[v] = 2 * idx.texcoord_index;
				}
				triangle.materialId = shape.mesh.material_ids[f];
				m_Triangles.push_back(std::move(triangle));

				index_offset += fv;
			}
		}
	}
}