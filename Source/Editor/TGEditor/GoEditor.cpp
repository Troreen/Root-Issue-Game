#include "stdafx.h"
#include "GoEditor.h"

#include <tge/input/InputManager.h>

#include <tge/drawers/SpriteDrawer.h>
#include <tge/shaders/SpriteShader.h>
#include <tge/texture/TextureManager.h>

#include <tge/script/ScriptNodeTypeRegistry.h>
#include <tge/settings/settings.h>

#include "Editor.h"

#include <tge/Script/Nodes/CommonNodes.h>

static const char* locSettingsPath;

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

void GoEditor(const char* aSettingsPath, const EditorConfiguration& aEditorConfiguration)
{
	locSettingsPath = aSettingsPath;

	Tga::LoadSettings(locSettingsPath);
	Tga::EngineConfiguration &cfg = Tga::Settings::GetEngineConfiguration();

	cfg.myWinProcCallback = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {return WinProc(hWnd, message, wParam, lParam); };
	cfg.myActivateDebugSystems = Tga::DebugFeature::Filewatcher;

	if (!Tga::Engine::Start())
	{
		ERROR_PRINT("Fatal error! Engine could not start!");
		system("pause");
		return;
	}
	
	{
		Tga::Engine& engine = *Tga::Engine::GetInstance();

		Tga::InputManager inputManager(*engine.GetHWND());
		SInputManager = &inputManager;

		Tga::Editor editor;
		editor.Init(aEditorConfiguration);

		while (engine.BeginFrame()) 
		{
			inputManager.Update();
			editor.Update(engine.GetDeltaTime(), inputManager);
			engine.EndFrame();
		}
	}

	Tga::Engine::GetInstance()->Shutdown();
}

