#pragma once

#include "scene/mesh.h"

namespace VK_Renderer
{
	struct MeshletOffset
	{
		uint32_t vertexOffset{ 0 };
		uint32_t primitiveOffset{ 0 };
		uint32_t materialOffset{ 0 };
	};

	struct Vertex
	{
		glm::vec4 position;
		glm::vec4 normal;
		glm::vec4 uv;
		glm::ivec4 materialId;
	};

	struct MeshletDescription
	{
		uint32_t vertexCount{ 0 };
		uint32_t primCount{ 0 };
		uint32_t vertexBegin{ 0 };
		uint32_t primBegin{ 0 };

		void Reset()
		{
			vertexCount = 0;
			primCount = 0;
			vertexBegin = 0;
			primBegin = 0;
		}
	};

	struct TextureAtlas
	{
		glm::ivec2 start{ 0, 0 };

		uint32_t width{ 0 };
		uint32_t height{ 0 };
		uint32_t id{ 0 };

		bool operator < (TextureAtlas const& other) const
		{
			return (start.x != other.start.x ? start.x < other.start.x : 
					start.y < other.start.y);
		}
	};

	class Meshlets
	{
	public:
		Meshlets() = default;
		Meshlets(uint16_t const& maxPrimitiveCount, 
				  uint16_t const& maxVertexCount);

		void Append(Mesh const& mesh);

		void CreateMaterialData();

	protected:
		uint16_t m_MaxPrimitiveCount;
		uint16_t m_MaxVertexCount;

		DeclareWithGetFunc(protected, std::vector<Vertex>, m, Vertices, const);
		DeclareWithGetFunc(protected, std::vector<MeshletDescription>, m, MeshletInfos, const);
		DeclareWithGetFunc(protected, std::vector<uint8_t>, m, PrimitiveIndices, const);
		DeclareWithGetFunc(protected, std::vector<uint32_t>, m, VertexIndices, const);
	
	public:
		TextureAtlas m_TextureAtlas;

		glm::ivec2 m_Extent{ 0, 0 };

		std::vector<unsigned char> m_TextureData;

		std::vector<Material> m_Materials;
	};
}