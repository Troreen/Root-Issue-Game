#pragma once

#include <CommonUtilities/Camera3D.hpp>
#include <CommonUtilities/InputHandler.h>
#include <CommonUtilities/Vector3.hpp>
#include <tge/graphics/Camera.h>

#include "FreeFlyCameraController.h"

#include <string>

class CameraSystem
{
public:
    CameraSystem();

    void Init();
    void SetSceneName(const std::string& aSceneName);
    void Update(float aDeltaTime);

    CommonUtilities::Camera3Df& GetCamera();
    const CommonUtilities::Camera3Df& GetCamera() const;

    Tga::Camera& GetRenderCamera();
    const Tga::Camera& GetRenderCamera() const;

    void SetCameraTransformFromScene(
        const CommonUtilities::Vector3<float>& aPosition,
        const CommonUtilities::Quaternion<float>& aRotation);

    void UpdateDebugCamera(float aDeltaTime, CommonUtilities::InputHandler& anInputHandler);

    void EnableFollowCameraFromPlayer(const CommonUtilities::Vector3<float>& aPlayerPosition);
    void UpdateFollowCamera(float aDeltaTime, const CommonUtilities::Vector3<float>& aPlayerPosition);
    void TriggerCameraShake(float aDurationSeconds, float anIntensityUnits);
    void ResetTransientEffects();

    bool IsFollowCameraEnabled() const;
    bool IsDebugCameraEnabled() const;

#ifdef _DEBUG
    void RenderDebugUi();
#endif

private:
    CommonUtilities::Vector3<float> ClampCameraPositionToBounds(const CommonUtilities::Vector3<float>& aPosition) const;
    CommonUtilities::Vector3<float> ComputeShakeOffset(float aDeltaTime);
    void SanitizeCameraBounds();

    CommonUtilities::Camera3Df myCamera;
    Tga::Camera myRenderCamera;

    float myDebugCamFovRadians;
    float myCameraNearPlane;
    float myCameraFarPlane;
    bool myFollowCameraEnabled;
    bool myDebugCameraEnabled;
    bool myFreeFlyCameraControllerInitialized;
    bool myF1HotkeyWasDown;
    float myCameraFollowAnchorX;
    float myCameraFollowAnchorY;
    bool myHasCameraFollowAnchor;
    bool myHasCameraFollowAnchorY;
    float myLastFollowPlayerPositionX;
    bool myHasLastFollowPlayerPositionX;

    CommonUtilities::Vector3<float> myFollowCamOffsetFromPlayer;
    float myFollowCamPitchRadians;
    float myFollowCamYawRadians;
    float myFollowCamRollRadians;
    float myFollowCamFovRadians;

    float myCameraHorizontalPadding;
    float myCameraVerticalPadding;
    float myCameraLookAheadDistance;
    float myCameraLerpSpeed;
    float myCameraDirectionSwitchSpeedThreshold;
    float myCurrentCameraLookAheadOffset;
    int myCameraMoveDirection;

    float mySwayTimeSeconds;
    float mySwayFrequencyHz;
    float mySwayAmplitudeUnits;
    float mySwayVerticalFactor;

    float myShakeDurationSeconds;
    float myShakeTimeRemainingSeconds;
    float myShakeElapsedSeconds;
    float myShakeIntensityUnits;
    float myShakeMaxIntensityUnits;
    float myShakeFrequencyHz;

    float myFreeFlyMoveSpeed;
    float myFreeFlyLookSensitivity;

    FreeFlyCameraController myFreeFlyCameraController;

    bool myCameraBoundsEnabled;
    CommonUtilities::Vector3<float> myCameraBoundsMin;
    CommonUtilities::Vector3<float> myCameraBoundsMax;
};
