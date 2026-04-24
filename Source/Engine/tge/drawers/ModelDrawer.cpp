#include "stdafx.h"

#include <tge/drawers/ModelDrawer.h>
#include <tge/shaders/ModelShader.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/DX11.h>
#include <tge/engine.h>
#include <tge/math/Matrix2x2.h>
#include <tge/model/AnimatedModelInstance.h>
#include <tge/model/ModelInstance.h>

using namespace Tga;

ModelDrawer::ModelDrawer()
{
}

ModelDrawer::~ModelDrawer()
{
}

bool ModelDrawer::Init()
{
	myDefaultShader = std::make_unique<ModelShader>();
	if (!myDefaultShader->Init())
	{
		return false;
	}

	myDefaultAnimatedModelShader = std::make_unique<ModelShader>();
	if (!myDefaultAnimatedModelShader->Init("shaders/animated_model_shader_VS", "shaders/model_shader_PS"))
	{
		return false;
	}

	myLambertShader = std::make_unique<ModelShader>();
	if (!myLambertShader->Init("Shaders/PbrModelShaderVS", "Shaders/LambertModelShaderPS"))
	{
		return false;
	}

	myLambertAnimatedModelShader = std::make_unique<ModelShader>();
	if (!myLambertAnimatedModelShader->Init("Shaders/AnimatedPbrModelShaderVS", "Shaders/LambertModelShaderPS"))
	{
		return false;
	}

	myPbrShader = std::make_unique<ModelShader>();
	if (!myPbrShader->Init("Shaders/PbrModelShaderVS", "Shaders/PbrModelShaderPS"))
	{
		return false;
	}

	myPbrAnimatedModelShader = std::make_unique<ModelShader>();
	if (!myPbrAnimatedModelShader->Init("Shaders/AnimatedPbrModelShaderVS", "Shaders/PbrModelShaderPS"))
	{
		return false;
	}

	return true;
}

void ModelDrawer::Draw(const AnimatedModelInstance& modelInstance)
{
	modelInstance.Render(*myDefaultAnimatedModelShader);
}

void ModelDrawer::Draw(const ModelInstance& modelInstance)
{
	modelInstance.Render(*myDefaultShader);
}

void ModelDrawer::DrawLambert(const AnimatedModelInstance& modelInstance)
{
	modelInstance.Render(*myLambertAnimatedModelShader);
}

void ModelDrawer::DrawLambert(const ModelInstance& modelInstance)
{
	modelInstance.Render(*myLambertShader);
}

void ModelDrawer::DrawPbr(const AnimatedModelInstance& modelInstance)
{
	modelInstance.Render(*myPbrAnimatedModelShader);
}

void ModelDrawer::DrawPbr(const ModelInstance& modelInstance)
{
	modelInstance.Render(*myPbrShader);
}

void ModelDrawer::Draw(const AnimatedModelInstance& modelInstance, const ModelShader& shader)
{
	modelInstance.Render(shader);
}

void ModelDrawer::Draw(const ModelInstance& modelInstance, const ModelShader& shader)
{
	modelInstance.Render(shader);
}