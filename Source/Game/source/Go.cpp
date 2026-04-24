#include "GameWorld.h"

#include <tge/input/InputManager.h>
#include <tge/scene/Scene.h>
#include <tge/scene/SceneSerialize.h>
#include <tge/settings/settings.h>
#include <tge/error/ErrorManager.h>
#include "Essentials.h"

LRESULT WinProc([[maybe_unused]]HWND hWnd, UINT message, [[maybe_unused]]WPARAM wParam, [[maybe_unused]]LPARAM lParam)
{
	if (Essentials::GetEssentials().globalInputManager->UpdateEvents(message, wParam, lParam))
	{
		return 0;
	}

	switch (message)
	{
		// this message is read when the window is closed
	case WM_DESTROY:
	{
		Essentials::Shutdown();
		// close the application entirely
		PostQuitMessage(0);
		return 0;
	}
	}
	return 0;
}


void Go(const char* argv[])
{
	Tga::LoadSettings(TGE_PROJECT_SETTINGS_FILE);

	Tga::EngineConfiguration& cfg = Tga::Settings::GetEngineConfiguration();

	cfg.myWinProcCallback = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {return WinProc(hWnd, message, wParam, lParam); };
#ifdef _DEBUG
	cfg.myActivateDebugSystems = Tga::DebugFeature::Fps | Tga::DebugFeature::Mem | Tga::DebugFeature::Filewatcher | Tga::DebugFeature::Cpu | Tga::DebugFeature::Drawcalls | Tga::DebugFeature::OptimizeWarnings;
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
		gameWorld.Init(argv);

		Tga::Engine& engine = *Tga::Engine::GetInstance();

		while (engine.BeginFrame())
		{
			Essentials::GetEssentials().globalInputManager->Update();
			gameWorld.Update(engine.GetDeltaTime(), argv);

			gameWorld.Render();

			engine.EndFrame();

			if (Essentials::ShutdownQueued)
			{
				break;
			}
		}
	}

	Tga::Engine::GetInstance()->Shutdown();
}

