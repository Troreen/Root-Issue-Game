#include "stdafx.h"
#include "ModelFactory.h"

#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>

#include <tge/animation/animationPlayer.h>
#include <tge/debug/LoadingProfiler.h>
#include <tge/graphics/DX11.h>
#include <tge/util/StringCast.h>
#include <tge/model/Model.h>
#include <tge/model/ModelInstance.h>
#include <tge/graphics/Vertex.h>
#include <tge/math/matrix4x4.h>
#include <tge/texture/texture.h>
#include <tge/texture/TextureManager.h>
#include <tge/filewatcher/FileWatcher.h>

#include <TGAFBXImporter/source/Importer.h>
#include <DDSTextureLoader/DDSTextureLoader11.h>

using namespace Tga;
ModelFactory* ModelFactory::myInstance = nullptr;

#define TEXTURE_SET_0 0
#define TEXTURE_SET_1 1
#define TEXTURE_SET_2 2
#define TEXTURE_SET_3 3

#define VERTEX_COLOR_SET_0 0
#define VERTEX_COLOR_SET_1 1
#define VERTEX_COLOR_SET_2 2
#define VERTEX_COLOR_SET_3 3

#define NUM_BONES_PER_VERTEX 4

using namespace Tga;

//Matrix4x4f ConvertToEngineMatrix33(const aiMatrix3x3& AssimpMatrix)
//{
//    Matrix4x4f mat;
//    for (int i = 0; i < 3; i++)
//    {
//        for (int j = 0; j < 3; j++)
//        {
//            mat(1 + i, 1 + j) = AssimpMatrix[i][j];
//        }
//    }
//
//    return mat;
//}
//
//// constructor from Assimp matrix
//Matrix4x4f ConvertToEngineMatrix4x4f(const aiMatrix4x4& AssimpMatrix)
//{
//    Matrix4x4f mat;
//    for (int i = 0; i < 4; i++)
//    {
//        for (int j = 0; j < 4; j++)
//        {
//            mat(1 + i, 1 + j) = AssimpMatrix[i][j];
//        }
//    }
//    return mat;
//}
//
//inline bool operator <(const aiString& aLhs, const aiString& aRhs)
//{
//    return strcmp(aLhs.data, aRhs.data) < 0;
//}

void AssignDefaultMaterials(const std::string& someFilePath, Model* aModel);

struct VertexBoneData
{
	unsigned int IDs[NUM_BONES_PER_VERTEX];
	float Weights[NUM_BONES_PER_VERTEX];

	VertexBoneData()
	{
		Reset();
	};

	void Reset()
	{
		memset(IDs, 0, sizeof(IDs));
		memset(Weights, 0, sizeof(Weights));
	}

	void AddBoneData(unsigned int BoneID, float Weight)
	{
		for (unsigned int i = 0; i < sizeof(IDs) / sizeof(IDs[0]); i++)
		{
			if (Weights[i] == 0.0)
			{
				IDs[i] = BoneID;
				Weights[i] = Weight;
				return;
			}
		}

		// should never get here - more bones than we have space for
		//assert(0);
	}
};

bool ModelFactory::InitUnitCube()
{
	// First we make a cube.

	Model::MeshData meshData = {};
	// Watch the winding! DX defaults to Clockwise.
	// Assume the winding as if you're viewing the face head on.
	// +Y up, +X right, +Z Forward
	meshData.Vertices = 
	{
		// Front
		{
			50.0f, -50.0f, 50.0f,
			0, 0, 1,
			1, 0, 0,
			0, -1, 0,
			1, 1, 1, 1,
			0, 1
		},
		{
			50.0f, 50.0f, 50.0f,
			0, 0, 1,
			1, 0, 0,
			0, -1, 0,
			1, 1, 1, 1,
			0, 0
		},
		{
			-50.0f, 50.0f, 50.0f,
			0, 0, 1,
			1, 0, 0,
			0, -1, 0,
			1, 1, 1, 1,
			1, 0
		},
		{
			-50.0f, -50.0f, 50.0f,
			0, 0, 1,
			1, 0, 0,
			0, -1, 0,
			1, 1, 1, 1,
			1, 1
		},

		// Left
		{
			-50.0f, -50.0f, 50.0f,
			-1, 0, 0,
			0, 0, 1,
			0, -1, 0,
			1, 0, 0, 1,
			0, 1
		},
		{
			-50.0f, 50.0f, 50.0f,
			-1, 0, 0,
			0, 0, 1,
			0, -1, 0,
			1, 0, 0, 1,
			0, 0
		},
		{
			-50.0f, 50.0f, -50.0f,
			-1, 0, 0,
			0, 0, 1,
			0, -1, 0,
			1, 0, 0, 1,
			1, 0
		},
		{
			-50.0f, -50.0f, -50.0f,
			-1, 0, 0,
			0, 0, 1,
			0, -1, 0,
			1, 0, 0, 1,
			1, 1
		},

		// Back
		{
			-50.0f, -50.0f, -50.0f,
			0, 0, -1,
			1, 0, 0,
			0, -1, 0,
			0, 1, 0, 1,
			0, 1
		},
		{
			-50.0f, 50.0f, -50.0f,
			0, 0, -1,
			1, 0, 0,
			0, -1, 0,
			0, 1, 0, 1,
			0, 0
		},
		{
			50.0f, 50.0f, -50.0f,
			0, 0, -1,
			1, 0, 0,
			0, -1, 0,
			0, 1, 0, 1,
			1, 0
		},
		{
			50.0f, -50.0f, -50.0f,
			0, 0, -1,
			1, 0, 0,
			0, -1, 0,
			0, 1, 0, 1,
			1, 1
		},

		// Right
		{
			50.0f, -50.0f, -50.0f,
			1, 0, 0,
			0, 0, -1,
			0, -1, 0,
			0, 0, 1, 1,
			0, 1
		},
		{
			50.0f, 50.0f, -50.0f,
			1, 0, 0,
			0, 0, -1,
			0, -1, 0,
			0, 0, 1, 1,
			0, 0
		},
		{
			50.0f, 50.0f, 50.0f,
			1, 0, 0,
			0, 0, -1,
			0, -1, 0,
			0, 0, 1, 1
			, 1, 0
		},
		{
			50.0f, -50.0f, 50.0f,
			1, 0, 0,
			0, 0, -1,
			0, -1, 0,
			0, 0, 1, 1,
			1, 1
		},

		// Top
		{
			50.0f, 50.0f, 50.0f,
			0, 1, 0,
			1, 0, 0,
			0, 0, 1,
			1, 1, 0, 1,
			0, 1
		},
		{
			50.0f, 50.0f, -50.0f,
			0, 1, 0,
			1, 0, 0,
			0, 0, 1,
			1, 1, 0, 1,
			0, 0
		},
		{
			-50.0f, 50.0f, -50.0f,
			0, 1, 0,
			1, 0, 0,
			0, 0, 1,
			1, 1, 0, 1,
			1, 0
		},
		{
			-50.0f, 50.0f, 50.0f,
			0, 1, 0,
			1, 0, 0,
			0, 0, 1,
			1, 1, 0, 1,
			1, 1
		},

		// Bottom
		{
			-50.0f, -50.0f, 50.0f,
			0, -1, 0,
			1, 0, 0,
			0, 0, 1,
			1, 0, 1, 1,
			0, 1
		},
		{
			-50.0f, -50.0f, -50.0f,
			0, -1, 0,
			1, 0, 0,
			0, 0, 1,
			1, 0, 1, 1,
			0, 0
		},
		{
			50.0f, -50.0f, -50.0f,
			0, -1, 0,
			1, 0, 0,
			0, 0, 1,
			1, 0, 1, 1,
			1, 0
		},
		{
			50.0f, -50.0f, 50.0f,
			0, -1, 0,
			1, 0, 0,
			0, 0, 1,
			1, 0, 1, 1
			, 1, 1
		},
	};

	meshData.Indices =
	{
		0, 1, 2,        /* |/ */
		0, 2, 3,        /* /| */
		4, 5, 6,        /* |/ */
		4, 6, 7,        /* /| */
		8, 9, 10,       /* |/ */
		8, 10, 11,      /* /| */
		12, 13, 14,     /* |/ */
		12, 14, 15,     /* /| */
		16, 17, 18,     /* |/ */
		16, 18, 19,     /* /| */
		20, 21, 22,     /* |/ */
		20, 22, 23      /* /| */
	};

	//const Vector3f extentsCenter = 0.5f * (minExtents + maxExtents);
	//const Vector3f boxExtents = 0.5f * (maxExtents - minExtents);
	//const float myBoxSphereRadius = FMath::Max(boxExtents.X, FMath::Max(boxExtents.Y, boxExtents.Z));

	HRESULT result;

	D3D11_BUFFER_DESC vertexBufferDesc{};
	vertexBufferDesc.ByteWidth = static_cast<UINT>(meshData.Vertices.size()) * static_cast<UINT>(sizeof(Vertex));
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexSubresourceData{};
	vertexSubresourceData.pSysMem = &meshData.Vertices[0];

	ID3D11Buffer* vertexBuffer;
	result = DX11::Device->CreateBuffer(&vertexBufferDesc, &vertexSubresourceData, &vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	D3D11_BUFFER_DESC indexBufferDesc{};
	indexBufferDesc.ByteWidth = static_cast<UINT>(meshData.Indices.size()) * static_cast<UINT>(sizeof(unsigned int));
	indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexSubresourceData{};
	indexSubresourceData.pSysMem = &meshData.Indices[0];

	ID3D11Buffer* indexBuffer;
	result = DX11::Device->CreateBuffer(&indexBufferDesc, &indexSubresourceData, &indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	std::shared_ptr<Model> model = std::make_shared<Model>();

	meshData.NumberOfVertices = static_cast<UINT>(meshData.Vertices.size());
	meshData.NumberOfIndices = static_cast<UINT>(meshData.Indices.size());
	meshData.Stride = sizeof(Vertex);
	meshData.Offset = 0;
	meshData.VertexBuffer = vertexBuffer;
	meshData.IndexBuffer = indexBuffer;
	meshData.Bounds = CalculateBoxSphereBounds(meshData.Vertices);
	model->Init(meshData, "Cube");

	AssignDefaultMaterials("", model.get());

	myLoadedModels.insert(std::pair<std::string, std::shared_ptr<Model>>("Cube", model));

	return true;
}

bool ModelFactory::InitUnitPlane()
{
	Model::MeshData meshData = {};

	meshData.Vertices.push_back({
		-50.0f, 0.0f, 50.0f,
		0, 1, 0,
		1, 0, 0,
		0, 0, 1,
		1, 1, 1, 1,
		0, 0
		});

	meshData.Vertices.push_back({
		50.0f, 0.0f, 50.0f,
		0, 1, 0,
		1, 0, 0,
		0, 0, 1,
		1, 1, 1, 1,
		1, 0
		});

	meshData.Vertices.push_back({
		50.0f, 0.0f, -50.0f,
		0, 1, 0,
		1, 0, 0,
		0, 0, 1,
		1, 1, 1, 1,
		1, 1
		});

	meshData.Vertices.push_back({
		-50.0f, 0.0f, -50.0f,
		0, 1, 0,
		1, 0, 0,
		0, 0, 1,
		1, 1, 1, 1,
		0, 1
		});


	meshData.Indices = { 0, 1, 2, 0, 2, 3 };

	//const Vector3f extentsCenter = 0.5f * (minExtents + maxExtents);
	//const Vector3f boxExtents = 0.5f * (maxExtents - minExtents);
	//const float myBoxSphereRadius = FMath::Max(boxExtents.X, FMath::Max(boxExtents.Y, boxExtents.Z));

	HRESULT result;

	D3D11_BUFFER_DESC vertexBufferDesc{};
	vertexBufferDesc.ByteWidth = static_cast<UINT>(meshData.Vertices.size()) * static_cast<UINT>(sizeof(Vertex));
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexSubresourceData{};
	vertexSubresourceData.pSysMem = &meshData.Vertices[0];

	ID3D11Buffer* vertexBuffer;
	result = DX11::Device->CreateBuffer(&vertexBufferDesc, &vertexSubresourceData, &vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	D3D11_BUFFER_DESC indexBufferDesc{};
	indexBufferDesc.ByteWidth = static_cast<UINT>(meshData.Indices.size()) * static_cast<UINT>(sizeof(unsigned int));
	indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexSubresourceData{};
	indexSubresourceData.pSysMem = &meshData.Indices[0];

	ID3D11Buffer* indexBuffer;
	result = DX11::Device->CreateBuffer(&indexBufferDesc, &indexSubresourceData, &indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	std::shared_ptr<Model> model = std::make_shared<Model>();

	meshData.NumberOfVertices = static_cast<UINT>(meshData.Vertices.size());
	meshData.NumberOfIndices = static_cast<UINT>(meshData.Indices.size());
	meshData.Stride = sizeof(Vertex);
	meshData.Offset = 0;
	meshData.VertexBuffer = vertexBuffer;
	meshData.IndexBuffer = indexBuffer;
	meshData.Bounds = CalculateBoxSphereBounds(meshData.Vertices);
	model->Init(meshData, "Plane");
	myLoadedModels.insert(std::pair<std::string, std::shared_ptr<Model>>("Plane", model));

	AssignDefaultMaterials("", model.get());

	return true;
}

bool ModelFactory::InitPrimitives()
{
	if (!InitUnitCube())
		return false;

	if (!InitUnitPlane())
		return false;

	return true;
}

ModelFactory::ModelFactory()
{
	TGA::FBX::Importer::InitImporter();
	InitPrimitives();
	myInstance = this;
}

ModelFactory::~ModelFactory()
{
	myInstance = nullptr;
	TGA::FBX::Importer::UninitImporter();
}

static TextureResource *AssignAlbedoTexture(const std::string& baseFileName, const std::string &materialFileName) 
{
	Engine& engine = *Engine::GetInstance();
	std::string fnAlbedo = materialFileName + "_C.dds"; 
	TextureResource	*albedoTexture = engine.GetTextureManager().TryGetTexture(fnAlbedo.c_str());

	if (albedoTexture == nullptr)
	{
		fnAlbedo = materialFileName + "_D.dds";
		albedoTexture = engine.GetTextureManager().TryGetTexture(fnAlbedo.c_str());
	}

	if (albedoTexture == nullptr)
	{
		fnAlbedo = baseFileName + "_C.dds";
		albedoTexture = engine.GetTextureManager().TryGetTexture(fnAlbedo.c_str());
	}

	if (albedoTexture == nullptr)
	{
		fnAlbedo = baseFileName + "_D.dds";
		albedoTexture = engine.GetTextureManager().TryGetTexture(fnAlbedo.c_str());
	}

	if (albedoTexture == nullptr)
	{
		albedoTexture = engine.GetTextureManager().GetTexture("Textures/T_Default_c.dds");
	}
	return albedoTexture;
}

static TextureResource *AssignNormalTexture(const std::string& baseFileName, const std::string &materialFileName) 
{
	Engine& engine = *Engine::GetInstance();
	std::string fnNormal = materialFileName + "_N.dds";
	TextureResource *normalTexture = engine.GetTextureManager().TryGetTexture(fnNormal.c_str(), TextureSrgbMode::ForceNoSrgbFormat);

	if (normalTexture == nullptr)
	{
		fnNormal = baseFileName + "_N.dds";
		normalTexture = engine.GetTextureManager().TryGetTexture(fnNormal.c_str(), TextureSrgbMode::ForceNoSrgbFormat);
	}

	if (normalTexture == nullptr)
		normalTexture = engine.GetTextureManager().GetTexture("Textures/T_Default_n.dds", TextureSrgbMode::ForceNoSrgbFormat);

	return normalTexture;
}

static TextureResource *AssignMaterialTexture(const std::string& baseFileName, const std::string &materialFileName) 
{
	Engine& engine = *Engine::GetInstance();
	std::string fnMaterial = materialFileName + "_M.dds";
	TextureResource *materialTexture = engine.GetTextureManager().TryGetTexture(fnMaterial.c_str(), TextureSrgbMode::ForceNoSrgbFormat);

	if (materialTexture == nullptr)
	{
		fnMaterial = baseFileName + "_M.dds";
		materialTexture = engine.GetTextureManager().TryGetTexture(fnMaterial.c_str(), TextureSrgbMode::ForceNoSrgbFormat);
	}

	if (materialTexture == nullptr)
		materialTexture = engine.GetTextureManager().GetTexture("Textures/T_Default_m.dds", TextureSrgbMode::ForceNoSrgbFormat);

	return materialTexture;
}

static TextureResource *AssignFxTexture(const std::string& baseFileName, const std::string &materialFileName) 
{
	Engine& engine = *Engine::GetInstance();
	std::string fnFx = materialFileName + "_FX.dds";
	TextureResource *fxTexture = engine.GetTextureManager().TryGetTexture(fnFx.c_str(), TextureSrgbMode::ForceNoSrgbFormat);

	if (fxTexture == nullptr)
	{
		fnFx = baseFileName + "_FX.dds";
		fxTexture = engine.GetTextureManager().TryGetTexture(fnFx.c_str(), TextureSrgbMode::ForceNoSrgbFormat);
	}

	if (fxTexture == nullptr)
		fxTexture = engine.GetTextureManager().GetTexture("Textures/T_Default_fx.dds", TextureSrgbMode::ForceNoSrgbFormat);
	return fxTexture;
}

void AssignDefaultMaterials(const std::string& someFilePath, Model* aModel)
{
	std::string baseFileName = someFilePath.substr(0, someFilePath.find_last_of(L'.'));
	std::string path = someFilePath.substr(0, someFilePath.find_last_of("/\\") + 1);

	for (int i = 0; i < aModel->GetMeshCount(); i++)
	{
		std::string filename = baseFileName.substr(baseFileName.find_last_of("/\\") + 1, baseFileName.length());
		std::string materialFileName = path + aModel->GetMaterialName(i);

		TextureResource* albedoTexture = AssignAlbedoTexture(baseFileName, materialFileName);
		aModel->SetDefaultTexture(i, 0, albedoTexture);

		TextureResource* normalTexture = AssignNormalTexture(baseFileName, materialFileName);
		aModel->SetDefaultTexture(i, 1, normalTexture);

		TextureResource* materialTexture = AssignMaterialTexture(baseFileName, materialFileName);
		aModel->SetDefaultTexture(i, 2, materialTexture);

		TextureResource* fxTexture = AssignFxTexture(baseFileName, materialFileName);
		aModel->SetDefaultTexture(i, 3, fxTexture);
	}
}


ModelInstance ModelFactory::GetModelInstance(const std::string& someFilePath)
{
	// This needs to be moved to separate memory structures at some point.
	ModelInstance meshInstance;

	std::shared_ptr<Model> model = GetModel(someFilePath);
	if (!model)
		return meshInstance;

	meshInstance.Init(model);

	return meshInstance;
		}

AnimatedModelInstance ModelFactory::GetAnimatedModelInstance(const std::string& someFilePath)
{
	// This needs to be moved to separate memory structures at some point.
	AnimatedModelInstance meshInstance;

	std::shared_ptr<Model> model = GetModel(someFilePath);
	if (!model || !model->GetSkeleton()->GetRoot())
		return meshInstance;

	meshInstance.Init(model);

	return meshInstance;
}

ModelInstance ModelFactory::GetUnitCube()
{
	return GetModelInstance("Cube");
}

ModelInstance ModelFactory::GetUnitPlane()
{
	return GetModelInstance("Plane");
}

std::shared_ptr<Model> ModelFactory::LoadModel(const std::string& someFilePath)
{
	// The FBX SDK doesn't like widechar :(.
	//const std::string ansiFileName = string_cast<std::string>(someFilePath);
	std::string resolved_path = Tga::Settings::ResolveAssetPath(someFilePath);
	if (resolved_path.empty())
		return nullptr;

	if (myWatchedPaths.find(someFilePath) == myWatchedPaths.end())
	{
		myWatchedPaths.insert(someFilePath);
		Engine::GetInstance()->GetFileWatcher()->WatchFileChange(resolved_path, std::bind(&Tga::ModelFactory::OnModelChanged, this, someFilePath));
	}

	TGA::FBX::Mesh tgaModel;
	if (TGA::FBX::Importer::LoadMeshA(resolved_path.data(), tgaModel))
	{
		Skeleton mdlSkeleton;

		if (tgaModel.Skeleton.GetRoot())
		{
			mdlSkeleton.Joints.resize(tgaModel.Skeleton.Bones.size());
			mdlSkeleton.JointNameToIndex.reserve(mdlSkeleton.Joints.size());
			mdlSkeleton.JointName.resize(mdlSkeleton.Joints.size());
			for (size_t j = 0; j < tgaModel.Skeleton.Bones.size(); j++)
			{
				Skeleton::Joint& mdlJoint = mdlSkeleton.Joints[j];
				TGA::FBX::Skeleton::Bone& tgaJoint = tgaModel.Skeleton.Bones[j];

				Matrix4x4f bindPoseInverseTranspose;
				memcpy(&bindPoseInverseTranspose, &tgaJoint.BindPoseInverse, sizeof(float) * 16);

				mdlJoint.BindPoseInverse = Matrix4x4f::Transpose(bindPoseInverseTranspose);

				mdlSkeleton.modelBindPose.JointTransforms[j] = mdlJoint.BindPoseInverse.GetInverse();

				mdlJoint.Name = tgaJoint.Name;
				mdlJoint.Parent = tgaJoint.ParentIdx;
				mdlJoint.Children = tgaJoint.Children;

				mdlSkeleton.JointNameToIndex.insert({ mdlJoint.Name, j });
				mdlSkeleton.JointName[j] = mdlJoint.Name;
			}
			assert(MAX_ANIMATION_BONES >= mdlSkeleton.Joints.size() && "More joints in animation than defined in EngingeDefines.h");

			mdlSkeleton.ConvertPoseToLocalSpace(mdlSkeleton.modelBindPose, mdlSkeleton.localBindPose);
		}

		std::vector<Model::MeshData> mdlMeshData;
		mdlMeshData.resize(tgaModel.Elements.size());

		// Convert model to our own format.
		for (size_t i = 0; i < tgaModel.Elements.size(); i++)
		{
			// The imported element data.
			TGA::FBX::Mesh::Element& element = tgaModel.Elements[i];

			// And where we'll put it in our structures.
			Model::MeshData& meshData = mdlMeshData[i];
			meshData.Vertices.resize(element.Vertices.size());

			// Convert vertices to your own format
			//std::vector<Vertex> mdlVertices;
			//mdlVertices.resize(element.Vertices.size());

			for (size_t v = 0; v < element.Vertices.size(); v++)
			{
				// The most important part, the position!
				meshData.Vertices[v].Position = {
					element.Vertices[v].Position[0],
					element.Vertices[v].Position[1],
					element.Vertices[v].Position[2],
					element.Vertices[v].Position[3]
				};

				// All four vertex color channels I have.
				for (int vCol = 0; vCol < 4; vCol++)
				{
					meshData.Vertices[v].VertexColors[vCol] = {
						element.Vertices[v].VertexColors[vCol][0],
						element.Vertices[v].VertexColors[vCol][1],
						element.Vertices[v].VertexColors[vCol][2],
						element.Vertices[v].VertexColors[vCol][3]
					};

					//FIX ME. Sets vertex colors to 1 if no data given
					// Daniel sager att FBX inportern laser vertex collors ratt, och om den laser fargerna ratt sa far den det vardet. 
					// Vilket leder till svart i vertex colors -> modeller blev transparenta.
					if (element.Vertices[v].VertexColors[vCol][0] + element.Vertices[v].VertexColors[vCol][1] + element.Vertices[v].VertexColors[vCol][2] + element.Vertices[v].VertexColors[vCol][3] == 0)
					{
						meshData.Vertices[v].VertexColors[vCol] = { 1,1,1,1 };
					}

				}

				meshData.Vertices[v].Normal = Vector3f(element.Vertices[v].Normal[0], element.Vertices[v].Normal[1], element.Vertices[v].Normal[2]);
				meshData.Vertices[v].Binormal = Vector3f(element.Vertices[v].BiNormal[0], element.Vertices[v].BiNormal[1], element.Vertices[v].BiNormal[2]);
				meshData.Vertices[v].Tangent = Vector3f(element.Vertices[v].Tangent[0], element.Vertices[v].Tangent[1], element.Vertices[v].Tangent[2]);

				for (unsigned int UVch = 0; UVch < 4; UVch++)
				{
					meshData.Vertices[v].UVs[UVch] = {
						 element.Vertices[v].UVs[UVch][0],
						 element.Vertices[v].UVs[UVch][1]
					};
				}

				meshData.Vertices[v].Bones = {
					static_cast<float>(element.Vertices[v].BoneIDs[0]),
					static_cast<float>(element.Vertices[v].BoneIDs[1]),
					static_cast<float>(element.Vertices[v].BoneIDs[2]),
					static_cast<float>(element.Vertices[v].BoneIDs[3])
				};

				meshData.Vertices[v].Weights = {
					element.Vertices[v].BoneWeights[0],
					element.Vertices[v].BoneWeights[1],
					element.Vertices[v].BoneWeights[2],
					element.Vertices[v].BoneWeights[3]
				};
			}

			//std::vector<unsigned int>& mdlIndices = element.Indices;
			meshData.Indices.resize(element.Indices.size());
			memcpy(meshData.Indices.data(), element.Indices.data(), sizeof(unsigned int) * element.Indices.size());
			//meshData.Indices = element.Indices;

			HRESULT result;

			D3D11_BUFFER_DESC vertexBufferDesc{};
			vertexBufferDesc.ByteWidth = static_cast<UINT>(meshData.Vertices.size()) * static_cast<UINT>(sizeof(Vertex));
			vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
			vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

			D3D11_SUBRESOURCE_DATA vertexSubresourceData{};
			vertexSubresourceData.pSysMem = &meshData.Vertices[0];

			ID3D11Buffer* vertexBuffer;
			result = DX11::Device->CreateBuffer(&vertexBufferDesc, &vertexSubresourceData, &vertexBuffer);
			if (FAILED(result))
			{
				return nullptr;
			}

			D3D11_BUFFER_DESC indexBufferDesc{};
			indexBufferDesc.ByteWidth = static_cast<UINT>(meshData.Indices.size()) * static_cast<UINT>(sizeof(float)); // TODO: What :P Sizeof should be uint.
			indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
			indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

			D3D11_SUBRESOURCE_DATA indexSubresourceData{};
			indexSubresourceData.pSysMem = &meshData.Indices[0];

			ID3D11Buffer* indexBuffer;
			result = DX11::Device->CreateBuffer(&indexBufferDesc, &indexSubresourceData, &indexBuffer);
			if (FAILED(result))
			{
				return nullptr;
			}

			meshData.NumberOfVertices = static_cast<UINT>(meshData.Vertices.size());
			meshData.NumberOfIndices = static_cast<UINT>(meshData.Indices.size());
			meshData.Stride = sizeof(Vertex);
			meshData.Offset = 0;
			meshData.VertexBuffer = vertexBuffer;
			meshData.IndexBuffer = indexBuffer;
			meshData.Name = element.MeshName;
			if (tgaModel.Materials.size() > element.MaterialIndex)
			{
				meshData.MaterialName = tgaModel.Materials[element.MaterialIndex].MaterialName;
			}
			else
			{
				meshData.MaterialName = "";
			}
			meshData.Bounds = CalculateBoxSphereBounds(meshData.Vertices);
		}

		std::shared_ptr<Model> model = std::make_shared<Model>();

		model->Init(mdlMeshData, resolved_path);
		if (mdlSkeleton.Joints.size() > 0)
		{
			model->mySkeleton = std::move(mdlSkeleton);
		}
		AssignDefaultMaterials(resolved_path, model.get());
		myLoadedModels.insert(std::pair<std::string, std::shared_ptr<Model>>(someFilePath, model));

		return model;
	}

	return nullptr;
}

Tga::BoxSphereBounds Tga::ModelFactory::CalculateBoxSphereBounds(std::vector<Tga::Vertex> somePositions)
{
	Vector3f minExtents = FLT_MAX;
	Vector3f maxExtents = -FLT_MAX;

	for (unsigned int v = 0; v < somePositions.size(); v++)
	{
		if (somePositions[v].Position.x > maxExtents.X)
			maxExtents.X = somePositions[v].Position.x;
		if (somePositions[v].Position.y > maxExtents.Y)
			maxExtents.Y = somePositions[v].Position.y;
		if (somePositions[v].Position.z > maxExtents.Z)
			maxExtents.Z = somePositions[v].Position.z;

		if (somePositions[v].Position.x < minExtents.X)
			minExtents.X = somePositions[v].Position.x;
		if (somePositions[v].Position.y < minExtents.Y)
			minExtents.Y = somePositions[v].Position.y;
		if (somePositions[v].Position.z < minExtents.Z)
			minExtents.Z = somePositions[v].Position.z;
	}

	const Vector3f extentsCenter = 0.5f * (minExtents + maxExtents);
	const Vector3f boxExtents = 0.5f * (maxExtents - minExtents);
	const float radius = boxExtents.Length();
	return { radius, boxExtents, extentsCenter };
}

std::shared_ptr<Animation> ModelFactory::GetAnimation(const std::string& someFilePath, const std::shared_ptr<Model>& aModel)
{
	const auto startTime = std::chrono::steady_clock::now();
	std::string resolvedPath = Tga::Settings::ResolveAssetPath(someFilePath);

	// The FBX SDK doesn't like widechar :(.
	TGA::FBX::Animation fbxAnimation;

	auto it = myLoadedAnimations.find(AnimationIdentifer{ resolvedPath , aModel });
	if (it != myLoadedAnimations.end())
	{
		const double milliseconds =
			std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTime).count();
		LoadingProfiler::GetInstance().RecordAnimationLoad(someFilePath, milliseconds, true);
		return it->second;
	}

	if (TGA::FBX::Importer::LoadAnimationA(resolvedPath, fbxAnimation))
	{
		std::shared_ptr<Animation> animation = std::make_shared<Animation>();
		animation->Name = string_cast<std::wstring>(fbxAnimation.Name);
		animation->Length = fbxAnimation.Length;
		animation->FramesPerSecond = fbxAnimation.FramesPerSecond;
		animation->Frames.resize(fbxAnimation.Frames.size());
		animation->Duration = static_cast<float>(fbxAnimation.Duration);

		const Tga::Skeleton& skeleton = *aModel->GetSkeleton();
		
		for (size_t f = 0; f < animation->Frames.size(); f++)
		{
			animation->Frames[f].Count = skeleton.Joints.size();
			for (const auto& [boneName, boneTransform] : fbxAnimation.Frames[f].LocalTransforms)
			{
				Matrix4x4f localMatrix;
				memcpy_s(&localMatrix, sizeof(Matrix4x4f), boneTransform.Data, sizeof(float) * 16);

				auto jointIt = skeleton.JointNameToIndex.find(boneName);

				if (jointIt != skeleton.JointNameToIndex.end())
					animation->Frames[f].JointTransforms[jointIt->second] = ScaleRotationTranslationf::CreateFromMatrix(localMatrix);
			}
		}

		myLoadedAnimations[AnimationIdentifer{ resolvedPath , aModel }] = animation;

		const double milliseconds =
			std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTime).count();
		LoadingProfiler::GetInstance().RecordAnimationLoad(someFilePath, milliseconds, false);
		return animation;
	}

	const double milliseconds =
		std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTime).count();
	LoadingProfiler::GetInstance().RecordAnimationLoad(someFilePath, milliseconds, false);
	return nullptr;
}

AnimationPlayer ModelFactory::GetAnimationPlayer(const std::string& someFilePath, const std::shared_ptr<Model>& aModel)
{
	LoadingProfiler::Scope scope("ModelFactory::GetAnimationPlayer");
	LoadingProfiler::GetInstance().RecordAnimationPath(someFilePath);
	AnimationPlayer instance;
	std::shared_ptr<Animation> animation = GetAnimation(someFilePath, aModel);
	if (animation)
		instance.Init(animation);

	return instance;
}

std::shared_ptr<Model> ModelFactory::GetModel(const std::string& someFilePath)
{
	const auto startTime = std::chrono::steady_clock::now();
	auto it = myLoadedModels.find(someFilePath);
	if (it != myLoadedModels.end())
	{
		const double milliseconds =
			std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTime).count();
		LoadingProfiler::GetInstance().RecordModelLoad(someFilePath, milliseconds, true);
		return it->second;
	}

	std::shared_ptr<Model> model = LoadModel(someFilePath);
	const double milliseconds =
		std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTime).count();
	LoadingProfiler::GetInstance().RecordModelLoad(someFilePath, milliseconds, false);
	return model;
}

void ModelFactory::OnModelChanged(const std::string& aUnresolvedPath)
{
	myLoadedModels.erase(aUnresolvedPath);
}
