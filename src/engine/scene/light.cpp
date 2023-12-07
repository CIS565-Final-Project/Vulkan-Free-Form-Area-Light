#include "light.h"
#include "tiny_obj_loader.h"
#include <iostream>

namespace VK_Renderer 
{
	AreaLight::AreaLight(AreaLightCreateInfo const& createInfo)
		: m_LightType(createInfo.type)
		, m_LightVertex(createInfo.lightVertex)
		, m_Transform(createInfo.transform)
		, m_BoundaryVertex(createInfo.boundPositions)
		, m_BoundaryUV(createInfo.boundUV)
		, m_BoundarySphere(createInfo.boundSphere)
		, m_Amplitude(createInfo.amplitude)
		, m_LightMaterial(createInfo.lightMaterial)
	{}

	AreaLight::AreaLight(const std::string& objfile)
		:m_LightType(LIGHT_TYPE::POLYGON)
		,m_Amplitude(1.f)
	{
		tinyobj::ObjReaderConfig reader_config;
		reader_config.triangulate = false;

		tinyobj::ObjReader reader;

		if (!reader.ParseFromFile(objfile.c_str(), reader_config)) {
			if (!reader.Error().empty()) {
				std::cerr << "TinyObjReader: " << reader.Error();
			}
			exit(1);
		}

		if (!reader.Warning().empty()) {
			std::cout << "TinyObjReader: " << reader.Warning();
		}

		auto& shapes = reader.GetShapes();

		auto& attrib = reader.GetAttrib();
		int vertexCnt = attrib.vertices.size() / 3;
		assert(vertexCnt == 4);
		std::vector<glm::vec3> tmpVertex(vertexCnt);
		std::memcpy(tmpVertex.data(), attrib.vertices.data(), attrib.vertices.size() * sizeof(tinyobj::real_t));

		int geomCnt = shapes.size();
		assert(geomCnt == 1);
		int faceCnt = shapes[0].mesh.num_face_vertices.size();
		assert(faceCnt == 1);
		int faceVertCnt = shapes[0].mesh.num_face_vertices[0];
		assert(faceVertCnt == 4);
		std::vector<int> vertexMap(faceVertCnt);
		std::vector<int> uvMap(faceVertCnt);
		for (int i = 0;i < faceVertCnt;++i) {
			tinyobj::index_t idx = shapes[0].mesh.indices[i];
			vertexMap[i] = idx.vertex_index;
			uvMap[i] = idx.texcoord_index;
		}

		//remap vertex back
		m_LightVertex.resize(vertexCnt);
		for (int i = 0;i < 4;++i) {
			m_LightVertex[vertexMap[i]] = tmpVertex[i];
			m_BoundaryVertex[vertexMap[i]] = tmpVertex[i];
		}

		if (attrib.texcoords.size() == 2 * 4) {
			std::vector<glm::vec2> tmpUV(vertexCnt);
			std::memcpy(tmpUV.data(), attrib.texcoords.data(), attrib.texcoords.size() * sizeof(tinyobj::real_t));
			for (int i = 0;i < 4;++i) {
				m_BoundaryUV[uvMap[i]] = tmpUV[i];
			}
		}
		else {
			m_BoundaryUV = { glm::vec2(1.f,0.f),glm::vec2(0.f,0.f),glm::vec2(0.f,1.f),glm::vec2(1.f,1.f) };
		}

		// load materials
		auto& materials = reader.GetMaterials();
		for (auto const& material : materials)
		{
			m_LightMaterial = MaterialInfo{
				.texPath = { material.diffuse_texname }
			};
		}
	}

	LightInfo AreaLight::GetLightInfo() const
	{
		LightInfo res;
		res.lightType = static_cast<int32_t>(m_LightType);
		res.arraySize = m_LightVertex.size();
		assert(res.arraySize <= MAX_LIGHT_VERTEX);
		glm::mat4 modelMatrix = m_Transform.GetTransformation();
		for (int i = 0;i < 4;++i) {
			res.boundUV[i] = m_BoundaryUV[i];
			res.boundPositions[i] = modelMatrix * glm::vec4(m_BoundaryVertex[i], 1.0);
		}
		for (int i = 0;i < m_LightVertex.size();++i) {
			res.lightVertex[i] = modelMatrix * glm::vec4(m_LightVertex[i], 1.0);
		}
		res.amplitude = m_Amplitude;
		{
			glm::vec4 transformedSphereCenter = modelMatrix * glm::vec4(m_BoundarySphere.x,m_BoundarySphere.y,m_BoundarySphere.z, 1.0);
			res.boundSphere = glm::vec4(transformedSphereCenter);
			float maxScale = 0.f;
			for (int i = 0;i < 3;++i) {
				maxScale = std::max(maxScale, abs(modelMatrix[i][i]));
			}
			res.boundSphere.w = m_BoundarySphere.w * maxScale;
		}
		return res;
	}


	//--------------------
	//SceneLight
	//--------------------
	AreaLight* SceneLight::GetLight(size_t idx)
	{
		if (idx > m_AreaLights.size())return nullptr;
		return &m_AreaLights[idx];
	}
	void SceneLight::AddLight(const AreaLight& lt)
	{
		m_AreaLights.emplace_back(lt);
		m_MaterialInfos.emplace_back(lt.m_LightMaterial);
	}
	std::vector<LightInfo> SceneLight::GetPackedLightInfo()
	{
		auto n = m_AreaLights.size();
		std::vector<LightInfo> ans(n);
		for (auto i = 0;i < n;++i) {
			ans[i] = m_AreaLights[i].GetLightInfo();
		}

		//update light inform after atlas
		auto compressedTex = GetLightTexture();//update light uvs
		auto& texBlocks = compressedTex.GetFinishedAtlas();
		for (int i = 0;i < texBlocks.size();++i) {
			TextureBlock2D const& atlas = texBlocks[i];
			glm::vec2 start = static_cast<glm::vec2>(atlas.start) / static_cast<glm::vec2>(compressedTex.GetResolution());
			glm::vec2 end = static_cast<glm::vec2>(atlas.start + glm::ivec2(atlas.width, atlas.height)) / static_cast<glm::vec2>(compressedTex.GetResolution());

			for (int j = 0;j < 4;++j) {
				glm::vec2 uv = ans[i].boundUV[j];
				uv = glm::mix(start, end, uv);
				ans[i].boundUV[j] = uv;
			}

		}

		return ans;
	}
	AtlasTexture2D SceneLight::GetLightTexture()
	{
		// Load textures
		std::vector<Material> materails;

		for (auto const& info : m_MaterialInfos)
		{
			materails.push_back(Material(info));
		}

		return AtlasTexture2D(materails);
	}
}