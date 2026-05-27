#pragma once

#include "CameraSystem.h"
#include "InputHandler.h"
#include "SceneRenderer.h"
#include "Timer.h"
#include "VfxSystem.h"
#include "tge/text/text.h"

#include <memory>
#include <string>
#include <vector>

class GameObject;

enum class eState
{
    eMainMenu,
    eOptions,
    ePlaying,
    eSplashScreen,
    eIntro,
    ePopState,
    ePopStack,
    eLoadInGameWithIntro,
    eLoadFirstLevel,
    eLoadSecondLevel,
    eLoadThirdLevel,
    eLoadFirstLog,
    eLoadSecondLog,
    COUNT
};

class State
{
public:
    virtual ~State();

    virtual void Init(CameraSystem& aCamera, const char* argv[]);
    virtual eState Update();
    virtual void Render();

    CommonUtilities::InputHandler& GetInputHandler();
    CommonUtilities::Timer& GetTimer();
    CommonUtilities::Camera3Df* GetCamera();

    GameObject* GetPlayer();
    void SetPlayer(GameObject* aPlayer);

    void SetCamera(CameraSystem& aCamera);
    CameraSystem* GetCameraSystem();

    float GetDeltaTime() const;

protected:
    void ApplyLoadedScene(std::vector<std::unique_ptr<GameObject>>&& someObjects, const std::string& aScenePath);
    void ClearSceneObjects();
    void RenderLoadingScreen();
    void RenderDefault();
    void TryRecoverWindowFocus();
    bool RestoreGameWindowFocusIfNeeded();

    std::vector<std::unique_ptr<GameObject>> myGameObjects;

    SceneRenderer mySceneRenderer;

    GameObject* myPlayer = nullptr;
    CameraSystem* myCameraSystem = nullptr;
    VfxSystem myVfxSystem;
    CommonUtilities::InputHandler myInputHandler;
    CommonUtilities::Timer myTimer;

    std::string mySceneName;

    int myPendingFocusRecoveryFrames = 0;

    bool myEnablePointLights = true;
    bool myEnableDirectionalLight = true;
    bool myEnableAmbientLight = true;

    Tga::Text myLoadingText;
};
