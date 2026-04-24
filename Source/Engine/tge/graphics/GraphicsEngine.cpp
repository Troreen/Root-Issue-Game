#include "stdafx.h"

#include <tge/graphics/GraphicsEngine.h>
#include <tge/engine.h>
#include <tge/drawers/CustomShapeDrawer.h>
#include <tge/drawers/LineDrawer.h>
#include <tge/drawers/ModelDrawer.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/graphics/Camera.h>
#include <tge/graphics/DX11.h>
#include <tge/graphics/FullscreenEffect.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/math/CommonMath.h>
#include <tge/render/RenderCommon.h>
#include <tge/render/RenderObject.h>
#include <tge/texture/texture.h>
#include <tge/texture/TextureManager.h>
#include <tge/windows/WindowsWindow.h>
#include <DDSTextureLoader/DDSTextureLoader11.h>
#include <tge/primitives/LinePrimitive.h>
#include <tge/EngineDefines.h>


#include <fstream>
#include <d3dcompiler.h>
#include <d3d11_1.h>
#include <dxgi.h>
#include <thread>

#include "FullscreenPixelateEffect.h"

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "D3DCompiler.lib")

using namespace Tga;


GraphicsEngine::GraphicsEngine()
	: myIsInitiated(false)
{}

GraphicsEngine::~GraphicsEngine(void)
{}

bool GraphicsEngine::Init()
{
	INFO_PRINT("%s", "Starting graphics engine");

	DX11::BackBuffer->SetAsActiveTarget();

	mySpriteDrawer = std::make_unique<SpriteDrawer>();
	mySpriteDrawer->Init();

	myModelDrawer = std::make_unique<ModelDrawer>();
	myModelDrawer->Init();

	myCustomShapeDrawer = std::make_unique<CustomShapeDrawer>();
	myCustomShapeDrawer->Init();

	myLineDrawer = std::make_unique<LineDrawer>();
	myLineDrawer->Init();

	myFullscreenCopy = std::make_unique<FullscreenEffect>();
	if (!myFullscreenCopy->Init("Shaders/PostprocessCopyPS"))
		return false;
	myFullscreenTonemap = std::make_unique<FullscreenEffect>();
	if (!myFullscreenTonemap->Init("Shaders/PostprocessTonemapPS"))
		return false;
	myFullscreenVerticalGaussianBlur = std::make_unique<FullscreenEffect>();
	if (!myFullscreenVerticalGaussianBlur->Init("Shaders/PostprocessGaussianV_PS"))
		return false;
	myFullscreenHorizontalGaussianBlur = std::make_unique<FullscreenEffect>();
	if (!myFullscreenHorizontalGaussianBlur->Init("Shaders/PostprocessGaussianH_PS"))
		return false;
	myFullscreenPixelateEffect = std::make_unique<FullscreenPixelateEffect>();
	if(!myFullscreenPixelateEffect->Init("Shaders/PostprocessPixelate_PS"))
		return false;

	myGraphicsStateStack = std::make_unique<GraphicsStateStack>();
	if (!myGraphicsStateStack->Init())
		return false;

	INFO_PRINT("%s", "All done, starting...");
	INFO_PRINT("%s", "#########################################");
	myIsInitiated = true;
	return true;
}

bool GraphicsEngine::IsInitiated()
{
	return myIsInitiated;
}

void Tga::GraphicsEngine::SetFullScreen(bool aFullScreen)
{
	DX11::SwapChain->SetFullscreenState(aFullScreen, nullptr);
}