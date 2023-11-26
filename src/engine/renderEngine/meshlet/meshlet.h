#pragma once

namespace VK_Renderer
{
	class Mesh;

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

	class Meshlets
	{
	public:
		Meshlets() = default;
		Meshlets(uint16_t const& maxPrimitiveCount, 
				  uint16_t const& maxVertexCount);

		void Append(Mesh const& mesh);

	protected:
		uint16_t m_MaterialOffset = 0;
		uint16_t m_MaxPrimitiveCount;
		uint16_t m_MaxVertexCount;

		DeclareWithGetFunc(protected, std::vector<Vertex>, m, Vertices, const);
		DeclareWithGetFunc(protected, std::vector<MeshletDescription>, m, MeshletInfos, const);
		DeclareWithGetFunc(protected, std::vector<uint8_t>, m, PrimitiveIndices, const);
		DeclareWithGetFunc(protected, std::vector<uint32_t>, m, VertexIndices, const);
	};
}