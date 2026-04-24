#include "stdafx.h"
#include "FullscreenEffect.h"

#include <fstream>

#include <tge/graphics/DX11.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>

using namespace Tga;

FullscreenEffect::FullscreenEffect()
{

}

bool FullscreenEffect::Init(const char* aPixelShaderPath)
{
	std::string vsData;
	myVertexShader = DX11::LoadVertexShader("Shaders/PostprocessVS");
	if (!myVertexShader)
		return false;

	myPixelShader = DX11::LoadPixelShader(aPixelShaderPath);

	if (!myPixelShader)
		return false;

	return true;
}

void FullscreenEffect::Render()
{
	auto& engine = *Tga::Engine::GetInstance();
	auto& graphicsStateStack = engine.GetGraphicsEngine().GetGraphicsStateStack();
	graphicsStateStack.UpdateGpuStates();

	DX11::Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DX11::Context->IASetInputLayout(nullptr);
	DX11::Context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	DX11::Context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

	DX11::Context->VSSetShader(myVertexShader->shader.Get(), nullptr, 0);
	DX11::Context->GSSetShader(nullptr, nullptr, 0);
	DX11::Context->PSSetShader(myPixelShader->shader.Get(), nullptr, 0);
	DX11::LogDrawCall();
	DX11::Context->Draw(3, 0);
}