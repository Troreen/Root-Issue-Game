#include "stdafx.h"
#include <tge/model/ModelInstance.h>
#include <tge/model/Model.h>
#include <tge/shaders/ModelShader.h>

using namespace Tga;

void ModelInstance::Init(std::shared_ptr<Model> aModel)
{
	myModel = aModel;

	for (int i = 0; i < MAX_MESHES_PER_MODEL; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			SetTexture(i, j, myModel->GetDefaultTextures(i)[j]);
		}
	}
}

std::shared_ptr<Model> ModelInstance::GetModel() const
{
	return myModel;
}

void ModelInstance::SetTransform(const Matrix4x4f& someTransform)
{
	myTransform = someTransform;
}

void ModelInstance::Render(const ModelShader& shader) const
{
	const std::vector<Model::MeshData>& meshData = myModel->GetMeshDataList();

	for (int j = 0; j < meshData.size(); j++)
	{
		shader.Render(myTextures[j], meshData[j], myTransform);
	}
}

void ModelInstance::Render(const ModelShader& shader, int aMeshIndex) const
{
	const std::vector<Model::MeshData>& meshData = myModel->GetMeshDataList();

	assert(aMeshIndex < meshData.size());
	if (aMeshIndex < meshData.size())
	{
		shader.Render(myTextures[aMeshIndex], meshData[aMeshIndex], myTransform);
	}
}
