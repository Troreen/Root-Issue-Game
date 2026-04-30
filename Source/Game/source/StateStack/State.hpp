#pragma once

#include "SceneRenderer.h"
#include "SceneImportService.h"
#include "CameraSystem.h"
#include "InputHandler.h"
#include "Essentials.h"
#include "VfxSystem.h"
#include "SceneObjectData.h"
#include "Timer.h"
#include "tge/text/text.h"
#include "GameObjectFactoryRegistrations.h"
#include "MeshComponent.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
#include <iostream>
#include <limits>

#include <Windows.h>

#include <tge/engine.h>
#include <tge/animation/Script/AnimationNodes.h>
#include <tge/error/ErrorManager.h>
#include <tge/graphics/DX11.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/script/Nodes/CommonMathNodes.h>
#include <tge/script/Nodes/CommonNodes.h>
#include "GameObject.h"

#include <tge/stringRegistry/StringRegistry.h>

enum class eState
{
    eMainMenu,
    eOptions,
    ePlaying,
    eSplashScreen,
    ePopState,
    ePopStack,
    COUNT
};

class State
{
public:

    virtual void Init(CameraSystem& aCamera, const char* argv[]) { myCameraSystem = &aCamera; UNREFERENCED_PARAMETER(argv); };
    virtual eState Update() { return eState::ePlaying; };
    virtual void Render() {};

    /// Get the input handler for processing input events.
    CommonUtilities::InputHandler& GetInputHandler() { return myInputHandler; };

    /// Get the timer for delta time and total elapsed time.
    CommonUtilities::Timer& GetTimer() { return myTimer;  };

    // Get the main camera for the game world.
    CommonUtilities::Camera3Df* GetCamera() { return &myCameraSystem->GetCamera(); };

    GameObject* GetPlayer() { return myPlayer; };
    void SetPlayer(GameObject* aPlayer) {myPlayer = aPlayer;};

    void SetCamera(CameraSystem& aCamera)
    {
		myCameraSystem = &aCamera;
    }

	CameraSystem* GetCameraSystem() { return myCameraSystem; }

    /// Get the current delta time (time since last frame in seconds).
    float GetDeltaTime() const { return myTimer.GetDeltaTime(); };

    virtual ~State() = default;

protected:
    void StartSceneLoadAsync(const std::string& aScenePath, bool aForceReload = false)
    {
        if (aScenePath.empty() || (!aForceReload && aScenePath == mySceneName))
        {
            return;
        }

        // Async loading is temporarily disabled. Keep the old implementation for easy rollback.
#if 0
        myVfxSystem.BeginSceneTransition(mySceneName, aScenePath);

        mySceneLoadTarget = aScenePath;

        myIsSceneLoading = true;
        mySceneLoadFuture = std::async(std::launch::async, [scenePath = aScenePath]()
            {
                SceneImportService importer;
                return importer.LoadSceneObjects(scenePath);
            });
#endif

        myVfxSystem.BeginSceneTransition(mySceneName, aScenePath);
        mySceneLoadTarget = aScenePath;
        myIsSceneLoading = false;
        LoadScene(aScenePath);
        mySceneLoadTarget.clear();

        myPendingFocusRecoveryFrames = 120;
        TryRecoverWindowFocus();
        if (Tga::Engine* engine = Tga::Engine::GetInstance(); engine && engine->GetHWND())
        {
            myInputHandler.SetWindowHandle(*engine->GetHWND());
        }
    };
    void ApplyLoadedScene(std::vector<std::unique_ptr<GameObject>>&& someObjects, const std::string& aScenePath)
    {
        mySceneName = aScenePath;
        myCameraSystem->SetSceneName(mySceneName);
        myCameraSystem->ResetTransientEffects();
        Essentials::globalSceneManager->SetCurrentScene(mySceneName);

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
    }
    void ClearSceneObjects()
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
    void RenderLoadingScreen()
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
    void RenderDefault()
    {
        mySceneRenderer.Render(
            myGameObjects,
            *myCameraSystem,
            myVfxSystem,
            myEnablePointLights,
            myEnableDirectionalLight,
            myEnableAmbientLight, true
            );

#ifdef _DEBUG
        myCameraSystem->RenderDebugUi();
#endif
    }
    void LoadScene(const std::string& aScenePath)
    {
        SceneImportService importer;
        auto importedObjects = importer.BuildGameObjects(aScenePath);
        ApplyLoadedScene(std::move(importedObjects), aScenePath);
    }
    void TryRecoverWindowFocus()
    {
        if (myPendingFocusRecoveryFrames <= 0)
        {
            return;
        }

        RestoreGameWindowFocusIfNeeded();
        --myPendingFocusRecoveryFrames;
    }
    bool RestoreGameWindowFocusIfNeeded()
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

        const bool hasForeground = (GetForegroundWindow() == hwnd);
        const bool hasFocus = (GetFocus() == hwnd) || (GetActiveWindow() == hwnd);
        return hasForeground && hasFocus;
    }

    std::vector<std::unique_ptr<GameObject>> myGameObjects;

    SceneRenderer mySceneRenderer;

    GameObject* myPlayer;
    CameraSystem* myCameraSystem;
    VfxSystem myVfxSystem;
    CommonUtilities::InputHandler myInputHandler;
    CommonUtilities::Timer myTimer;

    std::string mySceneName;
    std::future<std::vector<SceneObjectData>> mySceneLoadFuture;
    std::string mySceneLoadTarget;
    std::string myQueuedSceneRequest;

    bool myIsSceneLoading;
    int myPendingFocusRecoveryFrames;

    bool myEnablePointLights;
    bool myEnableDirectionalLight;
    bool myEnableAmbientLight;

    Tga::Text myLoadingText;
};