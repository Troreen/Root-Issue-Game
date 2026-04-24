#include "GameWorld.h"

#include <tge/input/InputManager.h>
#include <tge/scene/Scene.h>
#include <tge/scene/SceneSerialize.h>
#include <tge/settings/settings.h>
#include <tge/error/ErrorManager.h>

Tga::InputManager* SInputManager;

LRESULT WinProc(HWND /*hWnd*/, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (SInputManager->UpdateEvents(message, wParam, lParam)) {
		return 0;
	}

	switch (message)
	{
		// this message is read when the window is closed
		case WM_DESTROY:
		{
			// close the application entirely
			PostQuitMessage(0);
			return 0;
		}
	}

	return 0;
}


void Go(const char* aSceneToLoad)
{

	Tga::LoadSettings(TGE_PROJECT_SETTINGS_FILE);

	Tga::EngineConfiguration &cfg = Tga::Settings::GetEngineConfiguration();
	
	cfg.myWinProcCallback = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {return WinProc(hWnd, message, wParam, lParam); };
#ifdef _DEBUG
	cfg.myActivateDebugSystems = Tga::DebugFeature::Fps 
		| Tga::DebugFeature::Mem 
		| Tga::DebugFeature::Filewatcher 
		| Tga::DebugFeature::Cpu 
		| Tga::DebugFeature::Drawcalls 
		| Tga::DebugFeature::OptimizeWarnings;
#else
	cfg.myActivateDebugSystems = Tga::DebugFeature::Filewatcher;
#endif

	if (!Tga::Engine::Start())
	{
		ERROR_PRINT("Fatal error! Engine could not start!");
		system("pause");
		return;
	}
	
	{

		GameWorld gameWorld;
		gameWorld.Init();

		if (aSceneToLoad == nullptr)
			aSceneToLoad = "scene.tgs";

		Tga::Scene scene;
		Tga::LoadScene(aSceneToLoad, scene);
		gameWorld.LoadScene(scene);

		Tga::Engine& engine = *Tga::Engine::GetInstance();

		Tga::InputManager inputManager(*engine.GetHWND());
		SInputManager = &inputManager;

		while (engine.BeginFrame()) 
		{
			inputManager.Update();
			gameWorld.Update(engine.GetDeltaTime(), inputManager);
			gameWorld.Render();

			engine.EndFrame();
		}
	}

	Tga::Engine::GetInstance()->Shutdown();
}

