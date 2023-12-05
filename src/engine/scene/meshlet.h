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
		glm::vec4 boudningSphere{ 0 };
		uint32_t modelId;
		uint32_t vertexCount{ 0 };
		uint32_t primCount{ 0 };
		uint32_t vertexBegin{ 0 };
		uint32_t primBegin{ 0 };
		uint32_t padding[3];

		void Reset()
		{
			vertexCount = 0;
			primCount = 0;
			vertexBegin = 0;
			primBegin = 0;
		}
	};

	struct TextureBlock2D;

	class Meshlets
	{
	public:
		Meshlets() = default;
		Meshlets(uint16_t const& maxPrimitiveCount, 
				  uint16_t const& maxVertexCount);

		void Append(Mesh const& mesh, uint32_t const& modelId);

		inline std::vector<Vertex>& GetVertices() { return m_Vertices; }

	protected:
		static void ComputeBoundingSphere(MeshletDescription& meshletDesc, 
											std::vector<glm::vec3> const & vertices);

	protected:
		uint16_t m_MaxPrimitiveCount;
		uint16_t m_MaxVertexCount;

		uint16_t m_MaterialOffset{ 0 };

		DeclareWithGetFunc(protected, std::vector<Vertex>, m, Vertices, const);
		DeclareWithGetFunc(protected, std::vector<MeshletDescription>, m, MeshletInfos, const);
		DeclareWithGetFunc(protected, std::vector<uint8_t>, m, PrimitiveIndices, const);
		DeclareWithGetFunc(protected, std::vector<uint32_t>, m, VertexIndices, const);
	};
}