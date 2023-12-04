#include "testLayer.h"

#include <iostream>

#include <SDL.h>

#include "imgui.h"



using namespace VK_Renderer;

struct CameraUBO
{
	glm::vec4 pos;
	glm::mat4 viewProjMat;
};

struct MeshletInfo
{
	uint32_t Meshlet_Size;
	uint32_t Triangle_Count;
};

struct TriangleInfo
{
	glm::ivec4 pId;
	glm::ivec4 nId;
	glm::ivec4 uvId;
};

void CreateMeshPipeline(vk::Device vk_device,
	VK_GraphicsPipeline* pipeline,
	std::vector<vk::DescriptorSetLayout> const& descripotrSetLayouts,
	const std::string& task_shader_path,
	const std::string& mesh_shader_path,
	const std::string& frag_shader_path)
{
	// Create required shader stages
	auto task_shader = ReadFile(task_shader_path);
	auto mesh_shader = ReadFile(mesh_shader_path);
	auto frag_shader = ReadFile(frag_shader_path);

	vk::ShaderModule task_module = vk_device.createShaderModule(vk::ShaderModuleCreateInfo{
		.codeSize = task_shader.size(),
		.pCode = reinterpret_cast<const uint32_t*>(task_shader.data())
		});
	vk::ShaderModule mesh_module = vk_device.createShaderModule(vk::ShaderModuleCreateInfo{
		.codeSize = mesh_shader.size(),
		.pCode = reinterpret_cast<const uint32_t*>(mesh_shader.data())
		});
	vk::ShaderModule frag_module = vk_device.createShaderModule(vk::ShaderModuleCreateInfo{
		.codeSize = frag_shader.size(),
		.pCode = reinterpret_cast<const uint32_t*>(frag_shader.data())
		});

	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages
	{
		vk::PipelineShaderStageCreateInfo{
			.stage = vk::ShaderStageFlagBits::eTaskEXT,
			.module = task_module,
			.pName = "main"
		},
		vk::PipelineShaderStageCreateInfo{
			.stage = vk::ShaderStageFlagBits::eMeshEXT,
			.module = mesh_module,
			.pName = "main"
		},
		vk::PipelineShaderStageCreateInfo{
			.stage = vk::ShaderStageFlagBits::eFragment,
			.module = frag_module,
			.pName = "main"
		},
	};

	VK_PipelineInput input;
	input.SetupPipelineVertexInputCreateInfo();

	pipeline->CreatePipeline({.renderPass = Application::GetInstance()->GetRenderEngine()->GetRenderPass()->GetRenderPass()}, shader_stages, input, descripotrSetLayouts);

	vk_device.destroyShaderModule(task_module);
	vk_device.destroyShaderModule(mesh_module);
	vk_device.destroyShaderModule(frag_module);
}

RenderLayer::RenderLayer(std::string const& name, 
							std::string const&& meshFile,
							std::string const&& textureFile)
	: Layer(name), m_MeshFile(meshFile), m_TextureFile(textureFile)
{
}

void RenderLayer::OnAttach()
{
	m_Camera = mkU<PerspectiveCamera>();
	m_Camera->far = 500.f;
	m_Camera->m_Transform = Transformation{
		.position = {0, 0, 15},
	};
	m_Camera->m_Transform.Rotate(glm::pi<float>(), { 0, 1, 0 });
	m_Camera->resolution = { 680, 680 };
	m_Camera->RecomputeProjView();

	m_Engine = Application::GetInstance()->GetRenderEngine();
	m_Device = m_Engine->GetDevice();
	m_Swapchain = m_Engine->GetSwapchain();

	m_Cmd = mkU<VK_CommandBuffer>(m_Device->GetGraphicsCommandPool()->AllocateCommandBuffers({ .level = vk::CommandBufferLevel::eSecondary }));

	// Create Camera buffer
	m_CamBuffer = mkU<VK_StagingBuffer>(*m_Device);
	CameraUBO camera_ubo;
	camera_ubo.pos = glm::vec4(m_Camera.get()->GetTransform().position, 1);
	camera_ubo.viewProjMat = m_Camera->GetProjViewMatrix();
	m_CamBuffer->CreateFromData(&camera_ubo, sizeof(CameraUBO), vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive);

	// Create MaterialParam buffer
	float roughness = 1.f;
	m_MaterialParamBuffer = mkU<VK_StagingBuffer>(*m_Device);
	m_MaterialParamBuffer->CreateFromData(&roughness, sizeof(float), vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive);

	m_Scene = mkU<Scene>(); ;
	m_Scene->AddMesh("meshes/plane.obj", "Plane");
	m_Scene->AddMesh("meshes/wahoo.obj", "Wahoo",
		Transformation{
			.position = {0, 2, 7},
			.scale = {.5f, .5f, .5f}
		});
	m_Scene->ComputeRenderData({
		.MeshletMaxPrimCount = 32,
		.MeshletMaxVertexCount = 255
		});
	m_ModelMatrixBuffer = mkU<VK_StagingBuffer>(*m_Device);
	m_ModelMatrixBuffer->CreateFromData(m_Scene->GetModelMatries().data(), m_Scene->GetModelMatries().size() * sizeof(ModelMatrix), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);


	//Add lights
	m_SceneLight = mkU<SceneLight>();
	float halfWidth = 2.0f;
	std::vector<glm::vec3> polygon_verts = {
		glm::vec3(-halfWidth, -halfWidth + 1.0f, -5.0f),
		glm::vec3(halfWidth, -halfWidth + 1.0f, -5.0f),
		glm::vec3(halfWidth, halfWidth + 1.0f, -5.0f),
		glm::vec3(-halfWidth, halfWidth + 1.0f, -5.0f),
	};
	std::array<glm::vec3, 4> bound_verts = {
		glm::vec3(-halfWidth, -halfWidth + 1.0f, -5.0f),
		glm::vec3(halfWidth, -halfWidth + 1.0f, -5.0f),
		glm::vec3(halfWidth, halfWidth + 1.0f, -5.0f),
		glm::vec3(-halfWidth, halfWidth + 1.0f, -5.0f),
	};
	m_SceneLight->AddLight( AreaLight(LIGHT_TYPE::POLYGON,polygon_verts,bound_verts) );
	std::vector<glm::vec3> bezier_verts = {
		glm::vec3(-1.5, -0.5f, 5.0f),
		glm::vec3(1.5f, -0.5f, 5.0f),
		glm::vec3(10.5f, -0.5f, 5.0f),
		glm::vec3(1.5f, 2.5f, 5.0f),
		glm::vec3(1.5f, 2.5f, 5.0f),
		glm::vec3(-1.5f, 3.5f, 5.0f),
		glm::vec3(-2.5f, 2.5f, 5.0f),
		glm::vec3(-1.5f, -0.5f, 5.0f)
	};
	m_SceneLight->AddLight(AreaLight(LIGHT_TYPE::BEZIER,bezier_verts));
	m_LightBuffer = mkU<VK_StagingBuffer>(*m_Device);
	std::vector<LightInfo> lightBufferData = m_SceneLight->GetPackedLightInfo();
	m_LightBuffer->CreateFromData(lightBufferData.data(), lightBufferData.size() * sizeof(LightInfo), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
	auto idx = lightBufferData.size() * sizeof(LightInfo);
	m_LightCountBuffer = mkU<VK_StagingBuffer>(*m_Device);
	int light_count = lightBufferData.size();
   	m_LightCountBuffer->CreateFromData(&light_count, sizeof(int), vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive);


	Mesh lightMesh;
	lightMesh.LoadMeshFromFile("meshes/lightQuad.obj");

	m_LTCTexture = mkU<VK_Texture2D>(*m_Device);
	//m_LTCTexture->CreateFromFile("images/wahoo.bmp", { .format = vk::Format::eR8G8B8A8Unorm, .usage = vk::ImageUsageFlagBits::eSampled });
	m_LTCTexture->CreateFromFile("images/ltc.dds", { .format = vk::Format::eR32G32B32A32Sfloat, .usage = vk::ImageUsageFlagBits::eSampled });
	// m_LTCTexture->CreateFromFile("images/wall.jpg", { .format = vk::Format::eR8G8B8A8Unorm, .usage = vk::ImageUsageFlagBits::eSampled });
	m_LTCTexture->TransitionLayout(VK_ImageLayout{
		.layout = vk::ImageLayout::eShaderReadOnlyOptimal,
		.accessFlag = vk::AccessFlagBits::eShaderRead,
		.pipelineStage = vk::PipelineStageFlagBits::eFragmentShader,
		});

	m_LightTexture = mkU<VK_Texture2DArray>(*m_Device);
	m_LightBlurTexture = mkU<VK_Texture2DArray>(*m_Device);

	m_CompressedTexture = mkU<VK_Texture2DArray>(*m_Device);
	m_CompressedTexture->CreateFromData(m_Scene->GetAtlasTex2D()->GetData().data(),
		m_Scene->GetAtlasTex2D()->GetSize(),
		{
			.width = static_cast<uint32_t>(m_Scene->GetAtlasTex2D()->GetResolution().x),
			.height = static_cast<uint32_t>(m_Scene->GetAtlasTex2D()->GetResolution().y),
			.depth = 1,
		},
		{
			.format = vk::Format::eR8G8B8A8Unorm,
			.usage = vk::ImageUsageFlagBits::eSampled,
			.arrayLayer = 4
		}
		);

	m_CompressedTexture->TransitionLayout(VK_ImageLayout{
		.layout = vk::ImageLayout::eTransferDstOptimal,
		.accessFlag = vk::AccessFlagBits::eMemoryWrite,
		.pipelineStage = vk::PipelineStageFlagBits::eTransfer,
		});

	m_CompressedTexture->TransitionLayout(VK_ImageLayout{
		.layout = vk::ImageLayout::eShaderReadOnlyOptimal,
		.accessFlag = vk::AccessFlagBits::eShaderRead,
		.pipelineStage = vk::PipelineStageFlagBits::eFragmentShader,
		});

	m_LightBlurTexture->CreateFromFiles(
		//image files
		{
			"images/0.png",
			"images/1.png",
			"images/2.png",
			"images/3.png",
			"images/4.png",
			"images/5.png",
			"images/6.png",
			"images/7.png",
			"images/8.png"
		},
		//createInfo
		{
			.format = vk::Format::eR8G8B8A8Unorm,
			.usage = vk::ImageUsageFlagBits::eSampled,
			.arrayLayer = 9
		}
	);

	m_LightTexture->CreateFromFiles(
		//image files
		{
			"images/0.png",
		},
		//createInfo
		{
			.format = vk::Format::eR8G8B8A8Unorm,
			.usage = vk::ImageUsageFlagBits::eSampled,
			.arrayLayer = 1
		}
	);

	m_LightTexture->TransitionLayout(
		VK_ImageLayout{
			.layout = vk::ImageLayout::eShaderReadOnlyOptimal,
			.accessFlag = vk::AccessFlagBits::eShaderRead,
			.pipelineStage = vk::PipelineStageFlagBits::eFragmentShader,
		});

	m_LightBlurTexture->TransitionLayout(
		VK_ImageLayout{
			.layout = vk::ImageLayout::eShaderReadOnlyOptimal,
			.accessFlag = vk::AccessFlagBits::eShaderRead,
			.pipelineStage = vk::PipelineStageFlagBits::eFragmentShader,
		});

	std::vector<TriangleInfo> lightTriangles;
	for (auto& const tris : lightMesh.m_Triangles)
	{
		for (Triangle const& tri : tris)
		{
			lightTriangles.emplace_back(
				glm::ivec4(tri.pId, tri.materialId),
				glm::ivec4(tri.nId, 0),
				glm::ivec4(tri.uvId, 0)
			);
		}
	}

	//m_LTCMeshletInfo = mkU<MeshletInfo>(32, static_cast<uint32_t>(ltcMesh.GetTriangleCounts()));
	// m_LightMeshletInfo = mkU<MeshletInfo>(32, static_cast<uint32_t>(lightMesh.GetTriangleCounts()));
	// TEMP hardcode for beier curve
	m_LightMeshletInfo = mkU<MeshletInfo>(32, 100);

	// Create Buffers from meshlets
	m_MeshletInfoBuffer = mkU<VK_DeviceBuffer>(*m_Device);
	m_VertexIndicesBuffer = mkU<VK_DeviceBuffer>(*m_Device);
	m_PrimitiveIndicesBuffer = mkU<VK_DeviceBuffer>(*m_Device);
	m_VertexBuffer = mkU<VK_DeviceBuffer>(*m_Device);

	m_MeshletInfoBuffer->CreateFromData(m_Scene->GetMeshlets()->GetMeshletInfos().data(), sizeof(MeshletDescription) * m_Scene->GetMeshlets()->GetMeshletInfos().size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
	m_VertexIndicesBuffer->CreateFromData(m_Scene->GetMeshlets()->GetVertexIndices().data(), sizeof(uint32_t) * m_Scene->GetMeshlets()->GetVertexIndices().size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
	m_PrimitiveIndicesBuffer->CreateFromData(m_Scene->GetMeshlets()->GetPrimitiveIndices().data(), sizeof(uint8_t) * m_Scene->GetMeshlets()->GetPrimitiveIndices().size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
	m_VertexBuffer->CreateFromData(m_Scene->GetMeshlets()->GetVertices().data(), sizeof(Vertex) * m_Scene->GetMeshlets()->GetVertices().size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
	// Create Buffers

	auto generateMeshBufferSet = [=](MeshBufferSet& bufferSet, std::vector<TriangleInfo>& tris, Mesh& mesh, uPtr<MeshletInfo>& meshletInfo) -> void {
		bufferSet.m_MeshletInfoBuffer = mkU<VK_DeviceBuffer>(*m_Device);
		bufferSet.m_TriangleBuffer = mkU<VK_DeviceBuffer>(*m_Device);
		bufferSet.m_PositionBuffer = mkU<VK_DeviceBuffer>(*m_Device);
		bufferSet.m_NormalBuffer = mkU<VK_DeviceBuffer>(*m_Device);
		bufferSet.m_UVBuffer = mkU<VK_DeviceBuffer>(*m_Device);

		bufferSet.m_MeshletInfoBuffer->CreateFromData(meshletInfo.get(), sizeof(MeshletInfo), vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive);
		bufferSet.m_TriangleBuffer->CreateFromData(tris.data(), sizeof(TriangleInfo) * tris.size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
		bufferSet.m_PositionBuffer->CreateFromData(mesh.m_Positions.data(), sizeof(Float) * mesh.m_Positions.size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
		bufferSet.m_NormalBuffer->CreateFromData(mesh.m_Normals.data(), sizeof(Float) * mesh.m_Normals.size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
		bufferSet.m_UVBuffer->CreateFromData(mesh.m_UVs.data(), sizeof(Float) * mesh.m_UVs.size(), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive);
	};

	//generateMeshBufferSet(m_LTCMeshBufferSet, ltcTriangles, ltcMesh, m_LTCMeshletInfo);

	generateMeshBufferSet(m_LightMeshBufferSet, lightTriangles, lightMesh, m_LightMeshletInfo);


	// Create Descriptors
	m_MaterialParamDescriptor = mkU<VK_Descriptor>(*m_Device);
	m_MaterialParamDescriptor->Create({
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eUniformBuffer,
			.stage = vk::ShaderStageFlagBits::eFragment,
			.bufferInfo = vk::DescriptorBufferInfo{
				.buffer = m_MaterialParamBuffer->GetBuffer(),
				.offset = 0,
				.range = m_MaterialParamBuffer->GetSize()
			}
		}
		});

	m_CamDescriptor = mkU<VK_Descriptor>(*m_Device);
	m_CamDescriptor->Create({
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eUniformBuffer,
			.stage = vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment,
			.bufferInfo = vk::DescriptorBufferInfo{
				.buffer = m_CamBuffer->GetBuffer(),
				.offset = 0,
				.range = m_CamBuffer->GetSize()
			}
		}
		});

	m_LightDescriptor = mkU<VK_Descriptor>(*m_Device);
	m_LightDescriptor->Create(
		{
			VK_DescriptorBinding{
				.type = vk::DescriptorType::eUniformBuffer,
				.stage = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = m_LightCountBuffer->GetBuffer(),
					.offset = 0,
					.range = m_LightCountBuffer->GetSize(),
				}
			},
			VK_DescriptorBinding{
				.type = vk::DescriptorType::eStorageBuffer,
				.stage = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = m_LightBuffer->GetBuffer(),
					.offset = 0,
					.range = m_LightBuffer->GetSize(),
				}
			}
		}
	);

	auto generateDescriptorBinds = [](MeshBufferSet& bufferSet) -> std::vector<VK_DescriptorBinding> {
		return {
			VK_DescriptorBinding{
				.type = vk::DescriptorType::eUniformBuffer,
				.stage = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = bufferSet.m_MeshletInfoBuffer->GetBuffer(),
					.offset = 0,
					.range = bufferSet.m_MeshletInfoBuffer->GetSize()
				}
			},
			VK_DescriptorBinding{
				.type = vk::DescriptorType::eStorageBuffer,
				.stage = vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = bufferSet.m_TriangleBuffer->GetBuffer(),
					.offset = 0,
					.range = bufferSet.m_TriangleBuffer->GetSize()
				}
			},
			VK_DescriptorBinding{
				.type = vk::DescriptorType::eStorageBuffer,
				.stage = vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = bufferSet.m_PositionBuffer->GetBuffer(),
					.offset = 0,
					.range = bufferSet.m_PositionBuffer->GetSize()
				}
			},
			VK_DescriptorBinding{
				.type = vk::DescriptorType::eStorageBuffer,
				.stage = vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = bufferSet.m_NormalBuffer->GetBuffer(),
					.offset = 0,
					.range = bufferSet.m_NormalBuffer->GetSize()
				}
			},
			VK_DescriptorBinding{
				.type = vk::DescriptorType::eStorageBuffer,
				.stage = vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = bufferSet.m_UVBuffer->GetBuffer(),
					.offset = 0,
					.range = bufferSet.m_UVBuffer->GetSize()
				}
			},
		};
	};

	//std::vector<VK_DescriptorBinding> ltcMeshShaderDescriptorSet = generateDescriptorBinds(m_LTCMeshBufferSet);

	//std::vector<VK_DescriptorBinding> bindings{
	//	VK_DescriptorBinding{
	//			.type = vk::DescriptorType::eStorageBuffer,
	//			.stage = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT,
	//			.bufferInfo = vk::DescriptorBufferInfo{
	//				.buffer = m_MeshletInfoBuffer->GetBuffer(),
	//				.offset = 0,
	//				.range = m_MeshletInfoBuffer->GetSize()
	//			}
	//	},
	//	VK_DescriptorBinding{
	//			.type = vk::DescriptorType::eStorageBuffer,
	//			.stage = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT,
	//			.bufferInfo = vk::DescriptorBufferInfo{
	//				.buffer = m_ModelMatrixBuffer->GetBuffer(),
	//				.offset = 0,
	//				.range = m_ModelMatrixBuffer->GetSize()
	//			}
	//	},
	//	VK_DescriptorBinding{
	//			.type = vk::DescriptorType::eStorageBuffer,
	//			.stage = vk::ShaderStageFlagBits::eMeshEXT,
	//			.bufferInfo = vk::DescriptorBufferInfo{
	//				.buffer = m_VertexIndicesBuffer->GetBuffer(),
	//				.offset = 0,
	//				.range = m_VertexIndicesBuffer->GetSize()
	//			}
	//	},
	//	VK_DescriptorBinding{
	//			.type = vk::DescriptorType::eStorageBuffer,
	//			.stage = vk::ShaderStageFlagBits::eMeshEXT,
	//			.bufferInfo = vk::DescriptorBufferInfo{
	//				.buffer = m_PrimitiveIndicesBuffer->GetBuffer(),
	//				.offset = 0,
	//				.range = m_PrimitiveIndicesBuffer->GetSize()
	//			}
	//	},
	//	VK_DescriptorBinding{
	//			.type = vk::DescriptorType::eStorageBuffer,
	//			.stage = vk::ShaderStageFlagBits::eMeshEXT,
	//			.bufferInfo = vk::DescriptorBufferInfo{
	//				.buffer = m_VertexBuffer->GetBuffer(),
	//				.offset = 0,
	//				.range = m_VertexBuffer->GetSize()
	//			}
	//	},
	//	VK_DescriptorBinding{
	//		.type = vk::DescriptorType::eCombinedImageSampler,
	//		.stage = vk::ShaderStageFlagBits::eFragment,
	//		.imageInfo = vk::DescriptorImageInfo{
	//			.sampler = m_LTCTexture->GetSampler(),
	//			.imageView = m_LTCTexture->GetImageView(),
	//			.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
	//		}
	//	},
	//	VK_DescriptorBinding{
	//		.type = vk::DescriptorType::eCombinedImageSampler,
	//		.stage = vk::ShaderStageFlagBits::eFragment,
	//		.imageInfo = vk::DescriptorImageInfo{
	//		.sampler = m_LightTexture->GetSampler(),
	//		.imageView = m_LightTexture->GetImageView(),
	//		.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
	//		}
	//	},
	//	VK_DescriptorBinding{
	//		.type = vk::DescriptorType::eCombinedImageSampler,
	//		.stage = vk::ShaderStageFlagBits::eFragment,
	//		.imageInfo = vk::DescriptorImageInfo{
	//			.sampler = m_CompressedTexture->GetSampler(),
	//			.imageView = m_CompressedTexture->GetImageView(),
	//			.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
	//		}
	//	},
	//};

	//bindings.push_back(
	//});
	//bindings.push_back(
	//});
	//m_LTCMeshShaderInputDescriptor->Create(ltcMeshShaderDescriptorSet);
	//ltcMeshShaderDescriptorSet.push_back(VK_DescriptorBinding{
	//	.type = vk::DescriptorType::eCombinedImageSampler,
	//	.stage = vk::ShaderStageFlagBits::eFragment,
	//	.imageInfo = vk::DescriptorImageInfo{
	//		.sampler = m_Texture->GetSampler(),
	//		.imageView = m_Texture->GetImageView(),
	//		.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
	//	}
	//});
	
	//m_LTCMeshShaderInputDescriptor->Create(ltcMeshShaderDescriptorSet);
	//m_LTCMeshShaderInputDescriptor = mkU<VK_Descriptor>(*m_Device);
	//m_LTCMeshShaderInputDescriptor->Create(bindings);
	m_LTCMeshShaderInputDescriptor = mkU<VK_Descriptor>(*m_Device);
	m_LTCMeshShaderInputDescriptor->Create({
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eStorageBuffer,
			.stage = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT,
			.bufferInfo = vk::DescriptorBufferInfo{
				.buffer = m_MeshletInfoBuffer->GetBuffer(),
				.offset = 0,
				.range = m_MeshletInfoBuffer->GetSize()
			}
		},
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eStorageBuffer,
			.stage = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT,
			.bufferInfo = vk::DescriptorBufferInfo{
				.buffer = m_ModelMatrixBuffer->GetBuffer(),
				.offset = 0,
				.range = m_ModelMatrixBuffer->GetSize()
			}
		},
		VK_DescriptorBinding{
				.type = vk::DescriptorType::eStorageBuffer,
				.stage = vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = m_VertexIndicesBuffer->GetBuffer(),
					.offset = 0,
					.range = m_VertexIndicesBuffer->GetSize()
				}
		},
		VK_DescriptorBinding{
				.type = vk::DescriptorType::eStorageBuffer,
				.stage = vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = m_PrimitiveIndicesBuffer->GetBuffer(),
					.offset = 0,
					.range = m_PrimitiveIndicesBuffer->GetSize()
				}
		},
		VK_DescriptorBinding{
				.type = vk::DescriptorType::eStorageBuffer,
				.stage = vk::ShaderStageFlagBits::eMeshEXT,
				.bufferInfo = vk::DescriptorBufferInfo{
					.buffer = m_VertexBuffer->GetBuffer(),
					.offset = 0,
					.range = m_VertexBuffer->GetSize()
				}
		},
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eCombinedImageSampler,
			.stage = vk::ShaderStageFlagBits::eFragment,
			.imageInfo = vk::DescriptorImageInfo{
				.sampler = m_LTCTexture->GetSampler(),
				.imageView = m_LTCTexture->GetImageView(),
				.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
			}
		},
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eCombinedImageSampler,
			.stage = vk::ShaderStageFlagBits::eFragment,
			.imageInfo = vk::DescriptorImageInfo{
			.sampler = m_LightBlurTexture->GetSampler(),
			.imageView = m_LightBlurTexture->GetImageView(),
			.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
			}
		},
		VK_DescriptorBinding{
			.type = vk::DescriptorType::eCombinedImageSampler,
			.stage = vk::ShaderStageFlagBits::eFragment,
			.imageInfo = vk::DescriptorImageInfo{
				.sampler = m_CompressedTexture->GetSampler(),
				.imageView = m_CompressedTexture->GetImageView(),
				.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
			}
		},
		}
	);

	std::vector<VK_DescriptorBinding> lightMeshShaderDescriptorSet = generateDescriptorBinds(m_LightMeshBufferSet);
	
	lightMeshShaderDescriptorSet.push_back(VK_DescriptorBinding{
		.type = vk::DescriptorType::eCombinedImageSampler,
		.stage = vk::ShaderStageFlagBits::eFragment,
		.imageInfo = vk::DescriptorImageInfo{
			.sampler = m_LightTexture->GetSampler(),
			.imageView = m_LightTexture->GetImageView(),
			.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
		}
	});

	m_LightMeshShaderInputDescriptor = mkU<VK_Descriptor>(*m_Device);
	m_LightMeshShaderInputDescriptor->Create(lightMeshShaderDescriptorSet);

	// TEMP: Try two pipelines with same descriptor set
	// Create Pipeline
	m_MeshShaderLightPipeline = mkU<VK_GraphicsPipeline>(*m_Device);
	m_MeshShaderLTCPipeline = mkU<VK_GraphicsPipeline>(*m_Device);

	std::vector<vk::DescriptorSetLayout> ltc_descriptor_set_layouts{
		m_CamDescriptor->GetDescriptorSetLayout(),
		m_LTCMeshShaderInputDescriptor->GetDescriptorSetLayout(),
		m_LightDescriptor->GetDescriptorSetLayout(),
		m_MaterialParamDescriptor->GetDescriptorSetLayout(),
	};

	std::vector<vk::DescriptorSetLayout> light_descriptor_set_layouts{
		m_CamDescriptor->GetDescriptorSetLayout(),
		m_LightMeshShaderInputDescriptor->GetDescriptorSetLayout(),
		m_LightDescriptor->GetDescriptorSetLayout(),
	};
	//CreateMeshPipeline(m_Device->GetDevice(), m_MeshShaderLightPipeline.get(), light_descriptor_set_layouts,
	//	"shaders/mesh_flat.task.spv", "shaders/mesh_flat.mesh.spv", "shaders/mesh_flat.frag.spv");


	CreateMeshPipeline(m_Device->GetDevice(), m_MeshShaderLightPipeline.get(), light_descriptor_set_layouts,
		"shaders/mesh_light.task.spv", "shaders/mesh_light.mesh.spv", "shaders/mesh_light.frag.spv");
	CreateMeshPipeline(m_Device->GetDevice(), m_MeshShaderLTCPipeline.get(), ltc_descriptor_set_layouts,
		"shaders/mesh_ltc.task.spv", "shaders/mesh_ltc.mesh.spv", "shaders/mesh_ltc.frag.spv");

	RecordCmd();
	m_Engine->PushSecondaryCommandAll((*m_Cmd)[0]);
}

void RenderLayer::OnDetech()
{
}

void RenderLayer::OnUpdate(double const& deltaTime)
{
	
}

void RenderLayer::OnRender(double const& deltaTime)
{
}

void RenderLayer::OnImGui(double const& deltaTime)
{
	static float v = 1.f;
	ImGui::Begin("Test Window");
	if (ImGui::DragFloat("Roughness", &v, 0.02f, 0.f, 1.f))
	{
		m_MaterialParamBuffer->Update(&v, 0, sizeof(float));
	}
	ImGui::End();

	ImGui::Begin("Model Window");
	{
		static uint32_t current_id = 0;

		std::vector<MeshProxy>& meshes = m_Scene->GetMeshProxies();

		if (ImGui::BeginCombo("##combo", meshes[current_id].name.c_str()))
		{
			for (int i = 0; i < meshes.size(); ++i)
			{
				bool is_selected = (current_id == i);
				if (ImGui::Selectable(meshes[i].name.c_str(), is_selected))
				{
					current_id = i;
				}
			}
			ImGui::EndCombo();
		}

		bool modified = false;
		
		MeshProxy& mesh = meshes[current_id];
		glm::vec3 euler = glm::eulerAngles(mesh.transform.rotation);

		modified |= ImGui::DragFloat3("Position", &mesh.transform.position[0], 0.1f, -200.f, 200.f);
		modified |= ImGui::DragFloat3("Rotation", &euler[0], 0.1f, -2.f * glm::pi<float>(), 2.f * glm::pi<float>());
		modified |= ImGui::DragFloat3("Scale", &mesh.transform.scale[0], 0.01f, 0.01f, 1.f);
		
		if (modified)
		{
			mesh.transform.rotation = glm::quat(euler);
			m_Scene->UpdateModelMatrix(current_id);
			m_ModelMatrixBuffer->Update(&m_Scene->GetModelMatries()[mesh.id], sizeof(ModelMatrix) * mesh.id, sizeof(ModelMatrix));
		}
	}
	ImGui::End();

	ImGui::Begin("Light Window");
	{
		static uint32_t current_id = 0;

		std::vector<MeshProxy>& meshes = m_Scene->GetMeshProxies();
		std::vector<std::string> lightIdxs(m_SceneLight->GetLightCount());
		for (auto i = 0; i < m_SceneLight->GetLightCount();++i) {
			lightIdxs[i] = std::to_string(i);
		}
		//assert(lightIdxs.size() > 0);
		if (lightIdxs.size() > 0) {
			if (ImGui::BeginCombo("##combo", lightIdxs[0].c_str()))
			{
				for (int i = 0; i < lightIdxs.size(); ++i)
				{
					bool is_selected = (current_id == i);
					if (ImGui::Selectable(lightIdxs[i].c_str(), is_selected))
					{
						current_id = i;
					}
				}
				ImGui::EndCombo();
			}
		}


		bool modified = false;
		AreaLight& area_light = *m_SceneLight->GetLight(current_id);
		glm::vec3 euler = glm::eulerAngles(area_light.m_Transform.rotation);
		modified |= ImGui::DragFloat3("Position", &area_light.m_Transform.position[0], 0.1f, -200.f, 200.f);
		modified |= ImGui::DragFloat3("Rotation", &euler[0], 0.1f, -2.f * glm::pi<float>(), 2.f * glm::pi<float>());

		if (modified)
		{
			area_light.m_Transform.rotation = glm::quat(euler);
			LightInfo updatedInfo = area_light.GetLightInfo();
			m_LightBuffer->Update(&updatedInfo, sizeof(LightInfo) * current_id, sizeof(LightInfo));
		}
	}
	ImGui::End();
}

bool RenderLayer::OnEvent(SDL_Event const& e)
{
	static glm::ivec2 mouse_pre;
	if (e.type == SDL_MOUSEMOTION)
	{
		const Uint8* state = SDL_GetKeyboardState(nullptr);
		glm::ivec2 mouse_cur;
		SDL_GetMouseState(&mouse_cur.x, &mouse_cur.y);
		Uint32 mouseState = SDL_GetMouseState(nullptr, nullptr);
		if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
		{
			glm::vec2 offset = 0.001f * glm::vec2(mouse_cur - mouse_pre);
			m_Camera->m_Transform.Translate({ -offset.x, offset.y, 0 });
			m_Camera->RecomputeProjView();

			CameraUBO camera_ubo;
			camera_ubo.pos = glm::vec4(m_Camera.get()->GetTransform().position, 1);
			camera_ubo.viewProjMat = m_Camera->GetProjViewMatrix();
			m_CamBuffer->Update(&camera_ubo, 0, sizeof(CameraUBO));
		}
		if (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE))
		{
			glm::vec2 offset = 0.01f * glm::vec2(mouse_cur - mouse_pre);
			m_Camera->m_Transform.RotateAround(glm::vec3(0.f), { -offset.y, -offset.x, 0 });
			m_Camera->RecomputeProjView();
			
			CameraUBO camera_ubo;
			camera_ubo.pos = glm::vec4(m_Camera.get()->GetTransform().position, 1);
			camera_ubo.viewProjMat = m_Camera->GetProjViewMatrix();
			m_CamBuffer->Update(&camera_ubo, 0, sizeof(CameraUBO));
		}
		mouse_pre = mouse_cur;
	}
	if (e.type == SDL_WINDOWEVENT)
	{
		if (e.window.event == SDL_WINDOWEVENT_RESIZED)
		{
			m_Swapchain = m_Engine->GetSwapchain();
			RecordCmd();

			m_Camera->resolution = { e.window.data1, e.window.data2 };

			m_Camera->RecomputeProjView();

			CameraUBO camera_ubo;
			camera_ubo.pos = glm::vec4(m_Camera.get()->GetTransform().position, 1);
			camera_ubo.viewProjMat = m_Camera->GetProjViewMatrix();
			m_CamBuffer->Update(&camera_ubo, 0, sizeof(CameraUBO));
		}
		if (e.window.event == SDL_WINDOWEVENT_MAXIMIZED)
		{
			m_Swapchain = m_Engine->GetSwapchain();
			RecordCmd();

			int width, height;
			SDL_GetWindowSize(reinterpret_cast<SDL_Window*>(Application::GetInstance()->GetWindow()), &width, &height);
			m_Camera->resolution = { width, height };

			m_Camera->RecomputeProjView();

			CameraUBO camera_ubo;
			camera_ubo.pos = glm::vec4(m_Camera.get()->GetTransform().position, 1);
			camera_ubo.viewProjMat = m_Camera->GetProjViewMatrix();
			m_CamBuffer->Update(&camera_ubo, 0, sizeof(CameraUBO));
		}
	}
	return false;
}

void RenderLayer::RecordCmd()
{
	VK_CommandBuffer& cmd = *m_Cmd;
	cmd.Reset();
	{
		cmd.Begin({ .usage = vk::CommandBufferUsageFlagBits::eRenderPassContinue | vk::CommandBufferUsageFlagBits::eSimultaneousUse,
			.inheritInfo = {
				.renderPass = Application::GetInstance()->GetRenderEngine()->GetRenderPass()->GetRenderPass(),
				.subpass = 0}
		});



		// Viewport and scissors
		cmd[0].setViewport(0, vk::Viewport{
			.x = 0.f,
			.y = 0.f,
			.width = static_cast<float>(m_Swapchain->vk_ImageExtent.width),
			.height = static_cast<float>(m_Swapchain->vk_ImageExtent.height),
			.minDepth = 0.f,
			.maxDepth = 1.f
			});

		cmd[0].setScissor(0, vk::Rect2D{
			.offset = { 0, 0 },
			.extent = m_Swapchain->vk_ImageExtent
		});

		// draw LTC objects
		{
			cmd[0].bindPipeline(vk::PipelineBindPoint::eGraphics, m_MeshShaderLTCPipeline->GetPipeline());

			// Draw call
			uint32_t num_workgroups_x = m_Scene->GetMeshlets()->GetMeshletInfos().size();
			uint32_t num_workgroups_y = 1;
			uint32_t num_workgroups_z = 1;

			std::vector<vk::DescriptorSet> arr{
				m_CamDescriptor->GetDescriptorSet(),
				m_LTCMeshShaderInputDescriptor->GetDescriptorSet(),
				m_LightDescriptor->GetDescriptorSet(),
				m_MaterialParamDescriptor->GetDescriptorSet(),
			};


			cmd[0].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
				m_MeshShaderLTCPipeline->GetLayout(),
				uint32_t(0),
				arr, nullptr);

			cmd[0].drawMeshTasksEXT(num_workgroups_x, num_workgroups_y, num_workgroups_z);
		}

		// draw Light objects
		{
			cmd[0].bindPipeline(vk::PipelineBindPoint::eGraphics, m_MeshShaderLightPipeline->GetPipeline());

			// Draw call
			// uint32_t num_workgroups_x = (m_LightMeshletInfo->Triangle_Count + m_LightMeshletInfo->Meshlet_Size - 1) / m_LightMeshletInfo->Meshlet_Size;
			uint32_t num_workgroups_x = m_SceneLight->GetLightCount();
			uint32_t num_workgroups_y = 1;
			uint32_t num_workgroups_z = 1;

			std::vector<vk::DescriptorSet> arr{
				m_CamDescriptor->GetDescriptorSet(),
				m_LightMeshShaderInputDescriptor->GetDescriptorSet(),
				m_LightDescriptor->GetDescriptorSet()
			};


			cmd[0].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
				m_MeshShaderLightPipeline->GetLayout(),
				uint32_t(0),
				arr, nullptr);

			cmd[0].drawMeshTasksEXT(num_workgroups_x, num_workgroups_y, num_workgroups_z);
		}

		cmd.End();
	}
}
