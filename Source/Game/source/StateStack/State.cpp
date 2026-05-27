#include "State.hpp"

#include "Essentials.h"
#include "GameSettingsService.h"
#include "GameObject.h"
#include "SceneLoadingService.h"

#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <iostream>

#include <tge/debug/LoadingProfiler.h>
#include <tge/engine.h>
#include <tge/error/ErrorManager.h>
#include <tge/graphics/DX11.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>

State::~State() = default;

void State::Init(CameraSystem& aCamera, const char* argv[])
{
    myCameraSystem = &aCamera;
    (void)argv;
}

eState State::Update()
{
    return eState::ePlaying;
}

void State::Render()
{
}

CommonUtilities::InputHandler& State::GetInputHandler()
{
    return myInputHandler;
}

CommonUtilities::Timer& State::GetTimer()
{
    return myTimer;
}

CommonUtilities::Camera3Df* State::GetCamera()
{
    return myCameraSystem ? &myCameraSystem->GetCamera() : nullptr;
}

GameObject* State::GetPlayer()
{
    return myPlayer;
}

void State::SetPlayer(GameObject* aPlayer)
{
    myPlayer = aPlayer;
}

void State::SetCamera(CameraSystem& aCamera)
{
    myCameraSystem = &aCamera;
}

CameraSystem* State::GetCameraSystem()
{
    return myCameraSystem;
}

float State::GetDeltaTime() const
{
    return myTimer.GetDeltaTime();
}

void State::ApplyLoadedScene(std::vector<std::unique_ptr<GameObject>>&& someObjects, const std::string& aScenePath)
{
    Tga::LoadingProfiler::Scope scope("State::ApplyLoadedScene");

    mySceneName = aScenePath;
    myCameraSystem->SetSceneName(mySceneName);
    myCameraSystem->ResetTransientEffects();
    Essentials::globalSceneManager->SetCurrentScene(mySceneName);
    GameSettingsService::TryApplySceneLighting(mySceneName);

    ClearSceneObjects();

    Tga::Engine* engine = Tga::Engine::GetInstance();
    if (!engine)
    {
        ERROR_PRINT("GameWorld::ApplyLoadedScene failed to access engine instance.");
        return;
    }

    std::cout << "[GameWorld] Loaded " << someObjects.size() << " objects from scene: " << mySceneName << "\n";

    for (auto& object : someObjects)
    {
        if (!object)
        {
            continue;
        }

        object->Init(*engine);
        myGameObjects.push_back(std::move(object));
    }

    SceneLoadingService::FinishSceneApply();
}

void State::ClearSceneObjects()
{
    for (auto it = myGameObjects.begin(); it != myGameObjects.end();)
    {
        if (!(*it) || !(*it)->IsPersistent())
        {
            it = myGameObjects.erase(it);
            continue;
        }

        ++it;
    }
}

void State::RenderLoadingScreen()
{
    Tga::Engine* engine = Tga::Engine::GetInstance();
    if (!engine)
    {
        return;
    }

    auto& graphicsEngine = engine->GetGraphicsEngine();
    auto& graphicsStateStack = graphicsEngine.GetGraphicsStateStack();

    const int dotCount = static_cast<int>(std::fmod(myTimer.GetTotalTime() * 2.0, 4.0));
    std::string loadingText = "Loading";
    loadingText.append(static_cast<size_t>(dotCount), '.');

    const Tga::Vector2ui resolution = engine->GetRenderSize();
    myLoadingText.SetText(loadingText);
    myLoadingText.SetPosition({
        0.5f * static_cast<float>(resolution.x) - 0.5f * myLoadingText.GetWidth(),
        0.5f * static_cast<float>(resolution.y)
        });

    Tga::DX11::BackBuffer->SetAsActiveTarget();
    Tga::DX11::BackBuffer->Clear({ 0.0f, 0.0f, 0.0f, 1.0f });
    graphicsStateStack.SetDefaultCamera();
    myLoadingText.Render();
}

void State::RenderDefault()
{
    mySceneRenderer.Render(
        myGameObjects,
        *myCameraSystem,
        myVfxSystem,
        myEnablePointLights,
        myEnableDirectionalLight,
        myEnableAmbientLight,
        true);

#ifndef _RETAIL
    myCameraSystem->RenderDebugUi();
#endif
}

void State::TryRecoverWindowFocus()
{
    if (myPendingFocusRecoveryFrames <= 0)
    {
        return;
    }

    RestoreGameWindowFocusIfNeeded();
    --myPendingFocusRecoveryFrames;
}

bool State::RestoreGameWindowFocusIfNeeded()
{
    Tga::Engine* engine = Tga::Engine::GetInstance();
    if (!engine || !engine->GetHWND())
    {
        return false;
    }

    HWND hwnd = *engine->GetHWND();
    if (!hwnd)
    {
        return false;
    }

    const HWND foreground = GetForegroundWindow();

    if (foreground != hwnd)
    {
        const DWORD currentThread = GetCurrentThreadId();
        DWORD foregroundThread = 0;
        if (foreground)
        {
            foregroundThread = GetWindowThreadProcessId(foreground, nullptr);
        }

        if (foregroundThread != 0 && foregroundThread != currentThread)
        {
            AttachThreadInput(currentThread, foregroundThread, TRUE);
        }

        if (IsIconic(hwnd))
        {
            ShowWindow(hwnd, SW_RESTORE);
        }

        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
        SetFocus(hwnd);

        if (foregroundThread != 0 && foregroundThread != currentThread)
        {
            AttachThreadInput(currentThread, foregroundThread, FALSE);
        }
    }
    else
    {
        SetActiveWindow(hwnd);
        SetFocus(hwnd);
    }

    const bool hasForeground = GetForegroundWindow() == hwnd;
    const bool hasFocus = GetFocus() == hwnd || GetActiveWindow() == hwnd;
    return hasForeground && hasFocus;
}
