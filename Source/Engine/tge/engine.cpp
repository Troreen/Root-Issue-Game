#include "stdafx.h"

#include <tge/engine.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/debugging/MemoryTracker.h>
#include <tge/drawers/DebugDrawer.h>
#include <tge/error/ErrorManager.h>
#include <tge/filewatcher/FileWatcher.h>
#include <tge/graphics/dx11.h>
#include <tge/text/TextService.h>
#include <tge/texture/TextureManager.h>
#include <tge/model/ModelFactory.h>
#include <tge/windows/WindowsWindow.h>
#include <tge/settings/settings.h>

#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX 
#include <windows.h>
#include <tge/ImGui/ImGuiInterface.h>

#pragma comment( lib, "user32.lib" )

#ifndef _RETAIL
// Uncomment this define to use Live++:
// You also need to download Live++ and place the folder LivePP in Source/External
//#define USE_LIVE_PP
#endif

#ifdef USE_LIVE_PP
#include <LivePP/API/x64/LPP_API_x64_CPP.h>
static lpp::LppSynchronizedAgent locLppAgent;
static bool locIsLppValid;
#endif

namespace Tga
{
	void EnsureScenePropertiesAreLoaded();
	void EnsureBasePropertiesAreLoaded();

	void EnsureStaticInitializedTypesAreLoaded()
	{
		EnsureScenePropertiesAreLoaded();
		EnsureBasePropertiesAreLoaded();
	}
}

using namespace Tga; 
Engine* Tga::Engine::myInstance = nullptr;
Engine::Engine()
: myWindow(nullptr)
, myGraphicsEngine(nullptr)
, myTextureManager(nullptr)
, myDebugDrawer(nullptr)
, myRunEngine(true)
, myTotalTime(0.0f)
, myDeltaTime(0.0f)
, myShouldExit(false)
, myWantToUpdateSize(false)
{
	EnsureStaticInitializedTypesAreLoaded();

	{ // if specified for incomming parameters, prioritize
		myWindowConfiguration = Settings::GetEngineConfiguration();
		EngineConfiguration &cfg = myWindowConfiguration;
		{
			// if fullscreen we should not care what the settings say for the window size..
			if (cfg.myStartInFullScreen || cfg.myStartMaximized) {
				int screenWidth = GetSystemMetrics(SM_CXSCREEN);
				int screenHeight = GetSystemMetrics(SM_CYSCREEN);
				cfg.myWindowSize = cfg.myRenderSize = { (uint32_t)screenWidth, (uint32_t)screenHeight };
			}
		}
		
		myDebugDrawer = std::make_unique<DebugDrawer>(
			(myWindowConfiguration.myActivateDebugSystems & (
				DebugFeature::Fps | 
				DebugFeature::Mem | 
				DebugFeature::Drawcalls | 
				DebugFeature::Cpu | 
				DebugFeature::FpsGraph)) 
			!=  DebugFeature::None);

		myWindowConfiguration.myHwnd = cfg.myHwnd;
		myWindowConfiguration.myHInstance = cfg.myHInstance;
	}

	myErrorManager = std::make_unique<ErrorManager>();
	myErrorManager->AddLogListener(myWindowConfiguration.myLogFunction, myWindowConfiguration.myErrorFunction);
}


Engine::~Engine()
{}


void Tga::Engine::DestroyInstance()
{
	if (myInstance)
	{
#ifdef USE_LIVE_PP
		if (locIsLppValid)
		{
			lpp::LppDestroySynchronizedAgent(&locLppAgent);
		}
#endif
		delete myInstance;
		myInstance = nullptr;
		StopMemoryTrackingAndPrint();
	}
}

bool Engine::Start()
{
	if (!myInstance)
	{
		EngineConfiguration cfg = Settings::GetEngineConfiguration();
		MemoryTrackingSettings trackingSettings;
		trackingSettings.shouldTrackAllAllocations = ((cfg.myActivateDebugSystems & DebugFeature::MemoryTrackingAllAllocations) != DebugFeature::None);
		trackingSettings.shouldStoreStackTraces = ((cfg.myActivateDebugSystems & DebugFeature::MemoryTrackingStackTraces) != DebugFeature::None);
		StartMemoryTracking(trackingSettings);

		myInstance = new Engine();
		return myInstance->InternalStart();
	}
	else
	{
		ERROR_PRINT("%s", "DX2D::Engine::CreateInstance called twice, thats bad.");
	}
	return false;
}

bool Engine::InternalStart()
{
	INFO_PRINT("%s", "#########################################");
	INFO_PRINT("%s", "---TGE Starting, dream big and dare to fail---");

#ifdef USE_LIVE_PP
	locLppAgent = lpp::LppCreateSynchronizedAgent(nullptr, L"../Source/External/LivePP");
	locIsLppValid = lpp::LppIsValidSynchronizedAgent(&locLppAgent);
	locLppAgent.EnableModule(lpp::LppGetCurrentModulePath(), lpp::LPP_MODULES_OPTION_NONE, nullptr, nullptr);
#endif

	myFileWatcher = std::make_unique<FileWatcher>();
	myWindow = std::make_unique<WindowsWindow>();
	if (!myWindow->Init(myWindowConfiguration, myWindowConfiguration.myHInstance, myWindowConfiguration.myHwnd)) 
	{
		ERROR_PRINT("%s", "Window failed to be created!");
		return false;
	}

	myDx11 = std::make_unique<DX11>();
	if (!myDx11->Init(myWindow.get()))
	{
		ERROR_PRINT("%s", "D3D failed to be created!");
		myWindow->Close();
		return false;
	}

	myTextureManager = std::make_unique<TextureManager>();

	myGraphicsEngine = std::make_unique<GraphicsEngine>();
	if (!myGraphicsEngine->Init())
	{
		ERROR_PRINT("%s", "D3D engine failed to be created!");
		myWindow->Close();
		return false;
	}

	myTextureManager->Init();

	myTextService = std::make_unique<TextService>();
	myTextService->Init();

	CalculateRatios();

	if (myDebugDrawer)
	{
		myDebugDrawer->Init();
	}
#ifndef _RETAIL
	ImGuiInterface::Init();
#endif // !_RETAIL

	myTimeScale = 1.f;

	myStartOfTime = std::chrono::steady_clock::now();


	return true;
}

void Tga::Engine::Shutdown()
{
	ModelFactory::DestroyInstance();
	ImGuiInterface::Shutdown();

	if (myInstance)
	{
		DestroyInstance();
	}
}

void Tga::Engine::UpdateWindowSizeChanges()
{	
	myDx11->ResizeToWindowSize();
	DX11::BackBuffer->SetAsActiveTarget();

	myWindowConfiguration.myRenderSize = DX11::GetResolution();

	RECT r;
	GetWindowRect(*myWindowConfiguration.myHwnd, &r); //get window rect of control relative to screen
	myWindowConfiguration.myWindowSize = Vector2ui(r.right - r.left, r.bottom - r.top);

	CalculateRatios();
}

float Tga::Engine::GetRenderSizeRatio() const
{
	return myRenderSizeRatio;
}

float Tga::Engine::GetRenderSizeRatioInversed() const
{
	return myRenderSizeRatioInversed;
}

Vector2f Tga::Engine::GetRenderSizeRatioVec() const
{
	return myRenderSizeRatioVec;
}

Vector2f Tga::Engine::GetRenderSizeRatioInversedVec() const
{
	return myRenderSizeRatioInversedVec;
}

void Tga::Engine::SetResolution(const Vector2ui &aResolution)
{
	myWindow->SetResolution(aResolution);

	UpdateWindowSizeChanges();
}

void Tga::Engine::CalculateRatios()
{
	float sizeX = static_cast<float>(myWindowConfiguration.myRenderSize.x);
	float sizeY = static_cast<float>(myWindowConfiguration.myRenderSize.y);
	if (sizeY > sizeX)
	{
		float temp = sizeX;
		sizeX = sizeY;
		sizeY = temp;
	}

	myRenderSizeRatio = static_cast<float>(sizeX) / static_cast<float>(sizeY);
	myRenderSizeRatioInversed = static_cast<float>(sizeY) / static_cast<float>(sizeX);
	
	myRenderSizeRatioVec.x = 1.0f;
	myRenderSizeRatioVec.y = 1.0f;
	myRenderSizeRatioInversedVec.x = 1.0f;
	myRenderSizeRatioInversedVec.y = 1.0f;
	if (sizeX >= sizeY)
	{
		myRenderSizeRatioVec.y = myRenderSizeRatio;
		myRenderSizeRatioInversedVec.y = myRenderSizeRatioInversed;
	}
	else
	{
		myRenderSizeRatioVec.x = myRenderSizeRatio;
		myRenderSizeRatioInversedVec.x = myRenderSizeRatioInversed;
	}
}

HWND* Tga::Engine::GetHWND() const
{
	return myWindowConfiguration.myHwnd;
}


HINSTANCE Tga::Engine::GetHInstance() const
{
	return myWindowConfiguration.myHInstance;
}

void Tga::Engine::SetClearColor(const Color& aClearColor)
{
	myWindowConfiguration.myClearColor = aClearColor;
}

void Tga::Engine::SetFullScreen(bool aFullScreen)
{
	if (myGraphicsEngine)
	{
		myGraphicsEngine->SetFullScreen(aFullScreen);
	}
}

void Tga::Engine::SetBorderless(bool aBorderless)
{
	myWindow->SetBorderless(aBorderless);
	myIsBorderless = aBorderless;
	myWantToUpdateSize = true;
	UpdateWindowSizeChanges();
}

bool Tga::Engine::GetIsBorderless()
{
	return myIsBorderless;
}

void Tga::Engine::LockCursorToWindow()
{
	RECT rect;
	if (GetClientRect(*myWindowConfiguration.myHwnd, &rect))
	{
		POINT ul = { rect.left, rect.top };
		POINT lr = { rect.right, rect.bottom };

		ClientToScreen(*myWindowConfiguration.myHwnd, &ul);
		ClientToScreen(*myWindowConfiguration.myHwnd, &lr);

		rect.left = ul.x;
		rect.top = ul.y;
		rect.right = lr.x;
		rect.bottom = lr.y;

		ClipCursor(&rect);
	}
}

void Tga::Engine::UnlockCursor()
{
	ClipCursor(nullptr);
}

bool Tga::Engine::IsDebugFeatureOn(DebugFeature aFeature) const
{
	const bool all = ((myWindowConfiguration.myActivateDebugSystems & DebugFeature::All) == DebugFeature::All);
	if (all)
	{
		return true;
	}

	const bool specific = ((myWindowConfiguration.myActivateDebugSystems & aFeature) != DebugFeature::None);
	return specific;
}

bool Engine::BeginFrame()
{
	if (myShouldExit)
	{
		return false;
	}

#ifdef USE_LIVE_PP

	if (locIsLppValid && locLppAgent.WantsReload(lpp::LPP_RELOAD_OPTION_SYNCHRONIZE_WITH_RELOAD))
	{
		locLppAgent.Reload(lpp::LPP_RELOAD_BEHAVIOUR_WAIT_UNTIL_CHANGES_ARE_APPLIED);
	}

#endif

	MSG msg = { 0 };

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_QUIT)
		{
			INFO_PRINT("%s", "Exiting...");
			myShouldExit = true;
			return false;
		}
	}
#ifndef _RETAIL
	ImGuiInterface::PreFrame();
#endif // !_RETAIL
    myFileWatcher->FlushChanges();
	
	myDx11->BeginFrame(myWindowConfiguration.myClearColor);
    myTextureManager->Update();
	
	myGraphicsEngine->GetGraphicsStateStack().BeginFrame();

	DX11::ResetDrawCallCounter();

	return true;
}


void Engine::EndFrame( void )
{
#ifndef _RETAIL
	DX11::BackBufferNoSrgbConversion->SetAsActiveTarget();

	ImGuiInterface::Render();

	DX11::BackBuffer->SetAsActiveTarget();

#endif // !_RETAIL

	myTimer.Tick([&]()
	{
		myDeltaTime = static_cast<float>(myTimer.GetElapsedSeconds() * myTimeScale);
		myTotalTime += static_cast<float>(myTimer.GetElapsedSeconds() * myTimeScale);
		myUnscaledDeltaTime = static_cast<float>(myTimer.GetElapsedSeconds());
		myUnscaledTotalTime += static_cast<float>(myTimer.GetElapsedSeconds());
	});
	if (myDebugDrawer)
	{
		myDebugDrawer->Update(myDeltaTime);
		myDebugDrawer->Render();
	}

	myDx11->EndFrame(myWindowConfiguration.myEnableVSync);

	if (myWantToUpdateSize)
	{
		UpdateWindowSizeChanges();
		myWantToUpdateSize = false;
	}
}
