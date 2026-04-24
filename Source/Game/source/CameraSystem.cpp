#include "CameraSystem.h"

#include <tge/engine.h>

#include <algorithm>
#include <cmath>
#include <iostream>

#include "Essentials/Essentials.h"

#ifdef _DEBUG
#include "DebugSettings.h"
#include "imgui/imgui.h"
#endif

namespace
{
    constexpr float kPi = 3.14159265359f;
    constexpr float kDefaultCameraNearPlane = 1.0f;
    constexpr float kDefaultCameraFarPlane = 10000.0f;

    constexpr float DegreesToRadians(const float degrees)
    {
        return degrees * (kPi / 180.0f);
    }

    constexpr float RadiansToDegrees(const float radians)
    {
        return radians * (180.0f / kPi);
    }

    constexpr float kDefaultDebugCamFovDegrees = 95.0f;
    constexpr float kDefaultDebugCamFovRadians = DegreesToRadians(kDefaultDebugCamFovDegrees);

    constexpr float kFollowCamPitchDegrees = 5.5f;
    constexpr float kFollowCamPitchRadians = DegreesToRadians(kFollowCamPitchDegrees);
    constexpr float kFollowCamYawRadians = 0.0f;
    constexpr float kFollowCamRollRadians = 0.0f;
    constexpr float kFollowCamFovDegrees = 70.0f;
    constexpr float kFollowCamFovRadians = DegreesToRadians(kFollowCamFovDegrees);

    constexpr float kDebugCamFovMinDegrees = 20.0f;
    constexpr float kDebugCamFovMinRadians = DegreesToRadians(kDebugCamFovMinDegrees);
    constexpr float kDebugCamFovMaxDegrees = 160.5f;
    constexpr float kDebugCamFovMaxRadians = DegreesToRadians(kDebugCamFovMaxDegrees);

    constexpr float kDefaultFreeFlyMoveSpeed = 500.0f;
    constexpr float kDefaultFreeFlyLookSensitivity = 0.0025f;

    const CommonUtilities::Vector3<float> kFollowCamOffsetFromPlayer(346.0f, 506.0f, -4500.0f);

    void ApplyPerspectiveToCameras(
        CommonUtilities::Camera3Df& aGameCamera,
        Tga::Camera& aRenderCamera,
        const float aFovRadians,
        const float aNearPlane,
        const float aFarPlane,
        const Tga::Vector2ui& aResolution)
    {
        const float aspectRatio = static_cast<float>(aResolution.x) / static_cast<float>(aResolution.y);
        aGameCamera.SetPerspective(aFovRadians, aspectRatio, aNearPlane, aFarPlane);
        aRenderCamera.SetPerspectiveProjection(
            RadiansToDegrees(aFovRadians),
            { static_cast<float>(aResolution.x), static_cast<float>(aResolution.y) },
            aNearPlane,
            aFarPlane
        );
    }
}

CameraSystem::CameraSystem()
{
    myDebugCamFovRadians = 0.0f;
    myCameraNearPlane = 1500.0f;
    myCameraFarPlane = 20000.0f;
    myFollowCameraEnabled = true;
    myDebugCameraEnabled = false;
    myFreeFlyCameraControllerInitialized = false;
    myF1HotkeyWasDown = false;
    myCameraFollowAnchorX = 0.0f;
    myCameraFollowAnchorY = 0.0f;
    myHasCameraFollowAnchor = false;
    myHasCameraFollowAnchorY = false;
    myLastFollowPlayerPositionX = 0.0f;
    myHasLastFollowPlayerPositionX = false;

    myFollowCamOffsetFromPlayer = CommonUtilities::Vector3<float>(346.0f, 506.0f, -4500.0f);
    myFollowCamPitchRadians = 0.1f;
    myFollowCamYawRadians = 0.0f;
    myFollowCamRollRadians = 0.0f;
    myFollowCamFovRadians = 0.5f;

    myCameraHorizontalPadding = 86.7f;
    myCameraVerticalPadding = 78.6f;
    myCameraLookAheadDistance = 104.7f;
    myCameraLerpSpeed = 17.0f;
    myCameraDirectionSwitchSpeedThreshold = 60.0f;
    myCurrentCameraLookAheadOffset = 346.5f;
    myCameraMoveDirection = 1;

    mySwayTimeSeconds = 0.0f;
    mySwayFrequencyHz = 0.28f;
    mySwayAmplitudeUnits = 8.0f;
    mySwayVerticalFactor = 0.45f;

    myShakeDurationSeconds = 0.0f;
    myShakeTimeRemainingSeconds = 0.0f;
    myShakeElapsedSeconds = 0.0f;
    myShakeIntensityUnits = 0.0f;
    myShakeMaxIntensityUnits = 120.0f;
    myShakeFrequencyHz = 9.0f;

    myFreeFlyMoveSpeed = kDefaultFreeFlyMoveSpeed;
    myFreeFlyLookSensitivity = kDefaultFreeFlyLookSensitivity;

    myCameraBoundsEnabled = false;
    myCameraBoundsMin = CommonUtilities::Vector3<float>(-5000.0f, -5000.0f, -5000.0f);
    myCameraBoundsMax = CommonUtilities::Vector3<float>(5000.0f, 5000.0f, 5000.0f);
}

void CameraSystem::Init()
{
    myDebugCamFovRadians = kDefaultDebugCamFovRadians;
    myFollowCamPitchRadians = kFollowCamPitchRadians;
    myFollowCamYawRadians = kFollowCamYawRadians;
    myFollowCamRollRadians = kFollowCamRollRadians;
    myFollowCamFovRadians = kFollowCamFovRadians;
    myFollowCamOffsetFromPlayer = kFollowCamOffsetFromPlayer;

    myCameraNearPlane = kDefaultCameraNearPlane;
    myCameraFarPlane = kDefaultCameraFarPlane;

    myCameraLookAheadDistance = std::abs(myFollowCamOffsetFromPlayer.x);
    myCurrentCameraLookAheadOffset = myFollowCamOffsetFromPlayer.x;

    const Tga::Vector2ui resolution = Tga::Engine::GetInstance()->GetRenderSize();
    ApplyPerspectiveToCameras(myCamera, myRenderCamera, myDebugCamFovRadians, myCameraNearPlane, myCameraFarPlane, resolution);
}

void CameraSystem::SetSceneName(const std::string& aSceneName)
{
    (void)aSceneName;
}

void CameraSystem::Update(float aDeltaTime)
{
    if (false)
    {
        const CommonUtilities::Vector3<float> shakeOffset = ComputeShakeOffset(aDeltaTime);
        if (shakeOffset.LengthSqr() > 0.0f)
        {
            const CommonUtilities::Vector3<float> lockedPosition = myCamera.GetTransform().GetPosition();
            myCamera.GetTransform().SetPosition(ClampCameraPositionToBounds(lockedPosition + shakeOffset));
        }
    }

    myDebugCamFovRadians = std::clamp(myDebugCamFovRadians, kDebugCamFovMinRadians, kDebugCamFovMaxRadians);
    myFollowCamFovRadians = std::clamp(myFollowCamFovRadians, kDebugCamFovMinRadians, kDebugCamFovMaxRadians);
    myCameraFarPlane = std::clamp(myCameraFarPlane, 1.0f, kDefaultCameraFarPlane * 10.0f);
    myCameraNearPlane = std::clamp(myCameraNearPlane, 0.001f, std::max(0.002f, myCameraFarPlane - 0.001f));
    myCameraFarPlane = std::max(myCameraFarPlane, myCameraNearPlane + 0.001f);
    myCameraHorizontalPadding = std::max(0.0f, myCameraHorizontalPadding);
    myCameraVerticalPadding = std::max(0.0f, myCameraVerticalPadding);
    myCameraLookAheadDistance = std::max(0.0f, myCameraLookAheadDistance);
    myCameraLerpSpeed = std::max(0.01f, myCameraLerpSpeed);
    myCameraDirectionSwitchSpeedThreshold = std::max(0.0f, myCameraDirectionSwitchSpeedThreshold);
    SanitizeCameraBounds();

    const float activeFovRadians = myDebugCameraEnabled ? myDebugCamFovRadians : myFollowCamFovRadians;

    const Tga::Vector2ui resolution = Tga::Engine::GetInstance()->GetRenderSize();
    ApplyPerspectiveToCameras(myCamera, myRenderCamera, activeFovRadians, myCameraNearPlane, myCameraFarPlane, resolution);
}

CommonUtilities::Camera3Df& CameraSystem::GetCamera()
{
    return myCamera;
}

const CommonUtilities::Camera3Df& CameraSystem::GetCamera() const
{
    return myCamera;
}

Tga::Camera& CameraSystem::GetRenderCamera()
{
    return myRenderCamera;
}

const Tga::Camera& CameraSystem::GetRenderCamera() const
{
    return myRenderCamera;
}

void CameraSystem::SetCameraTransformFromScene(
    const CommonUtilities::Vector3<float>& aPosition,
    const CommonUtilities::Quaternion<float>& aRotation)
{
    myCamera.GetTransform().SetPosition(aPosition);
    myCamera.GetTransform().SetRotation(aRotation);
}

void CameraSystem::UpdateDebugCamera(float aDeltaTime, CommonUtilities::InputHandler& anInputHandler)
{
    const bool f1PressedFromInputHandler = anInputHandler.IsKeyPressed(Keys::F1);
    const bool rawF1Down = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
    const bool rawF1Pressed = rawF1Down && !myF1HotkeyWasDown;
    myF1HotkeyWasDown = rawF1Down;
    const bool backupTogglePressed =
        anInputHandler.IsKeyPressed(Keys::F4) ||
        anInputHandler.IsKeyPressed(Keys::OEM_3);

    if (f1PressedFromInputHandler || rawF1Pressed || backupTogglePressed)
    {
        myDebugCameraEnabled = !myDebugCameraEnabled;
        if (myDebugCameraEnabled)
        {
            myFreeFlyCameraControllerInitialized = false;
            std::cout << "[DebugCam] Enabled from hotkey\n";
        }
        else
        {
            myFreeFlyCameraController.ResetMouseLookAnchor();
            myFreeFlyCameraControllerInitialized = false;
            std::cout << "[DebugCam] Disabled from hotkey\n";
        }
    }

    if (!myDebugCameraEnabled)
    {
        return;
    }

    if (!myFreeFlyCameraControllerInitialized)
    {
        myFreeFlyCameraController.Init(anInputHandler, myCamera);
        myFreeFlyCameraControllerInitialized = true;
    }

    myFreeFlyCameraController.SetMoveSpeed(myFreeFlyMoveSpeed);
    myFreeFlyCameraController.SetLookSensitivity(myFreeFlyLookSensitivity);

    myFreeFlyCameraController.Update(aDeltaTime);
    myCamera.GetTransform().SetPosition(ClampCameraPositionToBounds(myCamera.GetTransform().GetPosition()));
}

void CameraSystem::EnableFollowCameraFromPlayer(const CommonUtilities::Vector3<float>& aPlayerPosition)
{
    myFollowCameraEnabled = true;
    const Tga::Vector2ui resolution = Tga::Engine::GetInstance()->GetRenderSize();
    const float activeFovRadians = myDebugCameraEnabled ? myDebugCamFovRadians : myFollowCamFovRadians;
    ApplyPerspectiveToCameras(myCamera, myRenderCamera, activeFovRadians, myCameraNearPlane, myCameraFarPlane, resolution);

    myCamera.GetTransform().SetYawPitchRollRadians(myFollowCamYawRadians, myFollowCamPitchRadians, myFollowCamRollRadians);

    myCameraFollowAnchorX = aPlayerPosition.x;
    myCameraFollowAnchorY = aPlayerPosition.y;
    myHasCameraFollowAnchor = true;
    myHasCameraFollowAnchorY = true;
    myLastFollowPlayerPositionX = aPlayerPosition.x;
    myHasLastFollowPlayerPositionX = true;

    myCameraMoveDirection = 1;
    myCurrentCameraLookAheadOffset = myCameraLookAheadDistance;

    CommonUtilities::Vector3<float> desiredCameraPosition(
        myCameraFollowAnchorX + myCurrentCameraLookAheadOffset,
        myCameraFollowAnchorY + myFollowCamOffsetFromPlayer.y,
        aPlayerPosition.z + myFollowCamOffsetFromPlayer.z);
    myCamera.GetTransform().SetPosition(ClampCameraPositionToBounds(desiredCameraPosition));
}

void CameraSystem::UpdateFollowCamera(float aDeltaTime, const CommonUtilities::Vector3<float>& aPlayerPosition)
{
	if (!myFollowCameraEnabled || myDebugCameraEnabled)
	{
		return;
	}

    if (!myHasCameraFollowAnchor)
    {
        myCameraFollowAnchorX = aPlayerPosition.x;
        myHasCameraFollowAnchor = true;
    }

    if (!myHasCameraFollowAnchorY)
    {
        myCameraFollowAnchorY = aPlayerPosition.y;
        myHasCameraFollowAnchorY = true;
    }

    if (!myHasLastFollowPlayerPositionX)
    {
        myLastFollowPlayerPositionX = aPlayerPosition.x;
        myHasLastFollowPlayerPositionX = true;
    }

    const float playerDeltaX = aPlayerPosition.x - myLastFollowPlayerPositionX;
    myLastFollowPlayerPositionX = aPlayerPosition.x;

    float playerSpeedX = 0.0f;
    if (aDeltaTime > 0.0f)
    {
        playerSpeedX = playerDeltaX / aDeltaTime;
    }

    if (playerSpeedX > myCameraDirectionSwitchSpeedThreshold)
    {
        myCameraMoveDirection = 1;
    }
    else if (playerSpeedX < -myCameraDirectionSwitchSpeedThreshold)
    {
        myCameraMoveDirection = -1;
    }

    if (aPlayerPosition.x > myCameraFollowAnchorX + myCameraHorizontalPadding)
    {
        myCameraFollowAnchorX = aPlayerPosition.x - myCameraHorizontalPadding;
    }
    else if (aPlayerPosition.x < myCameraFollowAnchorX - myCameraHorizontalPadding)
    {
        myCameraFollowAnchorX = aPlayerPosition.x + myCameraHorizontalPadding;
    }

    if (aPlayerPosition.y > myCameraFollowAnchorY + myCameraVerticalPadding)
    {
        myCameraFollowAnchorY = aPlayerPosition.y - myCameraVerticalPadding;
    }
    else if (aPlayerPosition.y < myCameraFollowAnchorY - myCameraVerticalPadding)
    {
        myCameraFollowAnchorY = aPlayerPosition.y + myCameraVerticalPadding;
    }

    const float desiredLookAheadOffset = static_cast<float>(myCameraMoveDirection) * myCameraLookAheadDistance;
    const float interpolation = std::clamp(1.0f - std::exp(-myCameraLerpSpeed * aDeltaTime), 0.0f, 1.0f);
    myCurrentCameraLookAheadOffset += (desiredLookAheadOffset - myCurrentCameraLookAheadOffset) * interpolation;

    CommonUtilities::Vector3<float> desiredCameraPosition(
        myCameraFollowAnchorX + myCurrentCameraLookAheadOffset,
        myCameraFollowAnchorY + myFollowCamOffsetFromPlayer.y,
        aPlayerPosition.z + myFollowCamOffsetFromPlayer.z);

    mySwayTimeSeconds += std::max(0.0f, aDeltaTime);
    const float swayPhase = mySwayTimeSeconds * mySwayFrequencyHz * 2.0f * kPi;
    CommonUtilities::Vector3<float> swayOffset(
        std::sin(swayPhase) * mySwayAmplitudeUnits,
        std::cos(swayPhase * 1.37f) * mySwayAmplitudeUnits * mySwayVerticalFactor,
        0.0f);

    const CommonUtilities::Vector3<float> shakeOffset = ComputeShakeOffset(aDeltaTime);
    const CommonUtilities::Vector3<float> finalOffset = swayOffset + shakeOffset;
    desiredCameraPosition += finalOffset;

    const auto currentCameraPosition = myCamera.GetTransform().GetPosition();
    const auto newCameraPosition = currentCameraPosition + (desiredCameraPosition - currentCameraPosition) * interpolation;
    myCamera.GetTransform().SetPosition(ClampCameraPositionToBounds(newCameraPosition));
    myCamera.GetTransform().SetYawPitchRollRadians(myFollowCamYawRadians, myFollowCamPitchRadians, myFollowCamRollRadians);
}

void CameraSystem::TriggerCameraShake(const float aDurationSeconds, const float anIntensityUnits)
{
    const float duration = std::max(0.01f, aDurationSeconds);
    const float intensity = std::max(0.0f, anIntensityUnits);

    myShakeDurationSeconds = std::max(myShakeDurationSeconds, duration);
    myShakeTimeRemainingSeconds = std::max(myShakeTimeRemainingSeconds, duration);
    myShakeIntensityUnits = std::clamp(myShakeIntensityUnits + intensity, 0.0f, myShakeMaxIntensityUnits);
    myShakeElapsedSeconds = 0.0f;
}

void CameraSystem::ResetTransientEffects()
{
    mySwayTimeSeconds = 0.0f;
    myShakeDurationSeconds = 0.0f;
    myShakeTimeRemainingSeconds = 0.0f;
    myShakeElapsedSeconds = 0.0f;
    myShakeIntensityUnits = 0.0f;
}

CommonUtilities::Vector3<float> CameraSystem::ComputeShakeOffset(const float aDeltaTime)
{
    if (myShakeTimeRemainingSeconds <= 0.0f || myShakeIntensityUnits <= 0.0f)
    {
        return CommonUtilities::Vector3<float>::Zero;
    }

    const float dt = std::max(0.0f, aDeltaTime);
    myShakeElapsedSeconds += dt;
    myShakeTimeRemainingSeconds = std::max(0.0f, myShakeTimeRemainingSeconds - dt);

    const float normalizedFade = (myShakeDurationSeconds > 0.0f)
        ? (myShakeTimeRemainingSeconds / myShakeDurationSeconds)
        : 0.0f;
    const float dampedIntensity = myShakeIntensityUnits * normalizedFade * normalizedFade;

    if (myShakeTimeRemainingSeconds <= 0.0f)
    {
        myShakeDurationSeconds = 0.0f;
        myShakeIntensityUnits = 0.0f;
    }

    const float shakePhase = myShakeElapsedSeconds * myShakeFrequencyHz * 2.0f * kPi;
    return {
        std::sin(shakePhase * 2.07f) * dampedIntensity,
        std::cos(shakePhase * 1.63f) * dampedIntensity * 0.7f,
        0.0f
    };
}

bool CameraSystem::IsFollowCameraEnabled() const
{
    return myFollowCameraEnabled;
}

bool CameraSystem::IsDebugCameraEnabled() const
{
    return myDebugCameraEnabled;
}

CommonUtilities::Vector3<float> CameraSystem::ClampCameraPositionToBounds(const CommonUtilities::Vector3<float>& aPosition) const
{
    if (!myCameraBoundsEnabled)
    {
        return aPosition;
    }

    CommonUtilities::Vector3<float> clamped = aPosition;
    clamped.x = std::clamp(clamped.x, myCameraBoundsMin.x, myCameraBoundsMax.x);
    clamped.y = std::clamp(clamped.y, myCameraBoundsMin.y, myCameraBoundsMax.y);
    clamped.z = std::clamp(clamped.z, myCameraBoundsMin.z, myCameraBoundsMax.z);
    return clamped;
}

void CameraSystem::SanitizeCameraBounds()
{
    if (myCameraBoundsMin.x > myCameraBoundsMax.x)
    {
        std::swap(myCameraBoundsMin.x, myCameraBoundsMax.x);
    }
    if (myCameraBoundsMin.y > myCameraBoundsMax.y)
    {
        std::swap(myCameraBoundsMin.y, myCameraBoundsMax.y);
    }
    if (myCameraBoundsMin.z > myCameraBoundsMax.z)
    {
        std::swap(myCameraBoundsMin.z, myCameraBoundsMax.z);
    }
}

#ifdef _DEBUG
void CameraSystem::RenderDebugUi()
{
    const auto cameraPos = myCamera.GetTransform().GetPosition();
    ImGui::TextUnformatted("Press F1 to toggle free look. Click the game window first.");
    ImGui::TextUnformatted("Move with WASD, rise with Shift, descend with Ctrl.");
    ImGui::Text("Camera Position: X %.2f  Y %.2f  Z %.2f", cameraPos.x, cameraPos.y, cameraPos.z);
    ImGui::Text("Near/Far: %.2f / %.2f", myCameraNearPlane, myCameraFarPlane);
    ImGui::Text("Free Look Active: %s", myDebugCameraEnabled ? "Yes" : "No");

    bool projectionChanged = false;
    bool boundsChanged = false;

    if (ImGui::Checkbox("Enable Debug Camera", &myDebugCameraEnabled))
    {
        if (myDebugCameraEnabled)
        {
            myFreeFlyCameraControllerInitialized = false;
            std::cout << "[DebugCam] Enabled from ImGui\n";
        }
        else
        {
            myFreeFlyCameraController.ResetMouseLookAnchor();
            myFreeFlyCameraControllerInitialized = false;
            std::cout << "[DebugCam] Disabled from ImGui\n";
        }
    }

    ImGui::SameLine();
    ImGui::Checkbox("Enable Follow Camera", &myFollowCameraEnabled);

    bool& showCollisionShapes = GameDebugSettings::ShowColliderDebugLines();
    ImGui::Checkbox("Show Collision Shapes", &showCollisionShapes);

    bool& enableCollisionDebugLog = GameDebugSettings::EnableCollisionDebugLog();
    ImGui::Checkbox("Log Collision Checks", &enableCollisionDebugLog);

    bool& logCollisionPairChecks = GameDebugSettings::LogCollisionPairChecks();
    ImGui::BeginDisabled(!enableCollisionDebugLog);
    ImGui::Checkbox("Log Collision Non-Hits", &logCollisionPairChecks);
    bool& logCollisionResolutionDetails = GameDebugSettings::LogCollisionResolutionDetails();
    ImGui::Checkbox("Log Collision Resolution Details", &logCollisionResolutionDetails);
    int& maxCollisionDebugLogs = GameDebugSettings::MaxCollisionDebugLogsPerFrame();
    ImGui::SliderInt("Collision Log Cap / Frame", &maxCollisionDebugLogs, 1, 500);
    ImGui::EndDisabled();

    bool& enableColliderDrawerDebugLog = GameDebugSettings::EnableColliderDrawerDebugLog();
    ImGui::Checkbox("Log Collider Drawers", &enableColliderDrawerDebugLog);

    ImGui::BeginDisabled(!enableColliderDrawerDebugLog);
    int& maxColliderDrawerDebugLogs = GameDebugSettings::MaxColliderDrawerDebugLogsPerFrame();
    ImGui::SliderInt("Collider Drawer Log Cap / Frame", &maxColliderDrawerDebugLogs, 1, 100);
    ImGui::EndDisabled();

    float debugFovDegrees = RadiansToDegrees(myDebugCamFovRadians);
    if (ImGui::SliderFloat("Debug FOV (deg)", &debugFovDegrees, kDebugCamFovMinDegrees, kDebugCamFovMaxDegrees, "%.1f"))
    {
        myDebugCamFovRadians = DegreesToRadians(debugFovDegrees);
        projectionChanged = true;
    }

    float followFovDegrees = RadiansToDegrees(myFollowCamFovRadians);
    if (ImGui::SliderFloat("Follow FOV (deg)", &followFovDegrees, kDebugCamFovMinDegrees, kDebugCamFovMaxDegrees, "%.1f"))
    {
        myFollowCamFovRadians = DegreesToRadians(followFovDegrees);
        projectionChanged = true;
    }

    projectionChanged |= ImGui::SliderFloat("Near Plane", &myCameraNearPlane, 0.001f, 1000.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
    projectionChanged |= ImGui::SliderFloat("Far Plane", &myCameraFarPlane, 1.0f, 500000.0f, "%.1f", ImGuiSliderFlags_Logarithmic);

    ImGui::SeparatorText("Follow Camera");
    ImGui::SliderFloat3("Follow Offset", &myFollowCamOffsetFromPlayer.x, -10000.0f, 10000.0f, "%.1f");

    float followYawDegrees = RadiansToDegrees(myFollowCamYawRadians);
    if (ImGui::SliderFloat("Follow Yaw (deg)", &followYawDegrees, -180.0f, 180.0f, "%.1f"))
    {
        myFollowCamYawRadians = DegreesToRadians(followYawDegrees);
    }

    float followPitchDegrees = RadiansToDegrees(myFollowCamPitchRadians);
    if (ImGui::SliderFloat("Follow Pitch (deg)", &followPitchDegrees, -89.0f, 89.0f, "%.1f"))
    {
        myFollowCamPitchRadians = DegreesToRadians(followPitchDegrees);
    }

    float followRollDegrees = RadiansToDegrees(myFollowCamRollRadians);
    if (ImGui::SliderFloat("Follow Roll (deg)", &followRollDegrees, -180.0f, 180.0f, "%.1f"))
    {
        myFollowCamRollRadians = DegreesToRadians(followRollDegrees);
    }

    ImGui::SliderFloat("Horizontal Padding", &myCameraHorizontalPadding, 0.0f, 2000.0f, "%.1f");
    ImGui::SliderFloat("Vertical Padding", &myCameraVerticalPadding, 0.0f, 2000.0f, "%.1f");
    ImGui::SliderFloat("Look Ahead Distance", &myCameraLookAheadDistance, 0.0f, 5000.0f, "%.1f");
    ImGui::SliderFloat("Camera Lerp Speed", &myCameraLerpSpeed, 0.01f, 30.0f, "%.2f");
    ImGui::SliderFloat("Direction Switch Threshold", &myCameraDirectionSwitchSpeedThreshold, 0.0f, 3000.0f, "%.1f");

    ImGui::SeparatorText("Free Look");
    ImGui::SliderFloat("Free Look Move Speed", &myFreeFlyMoveSpeed, 10.0f, 5000.0f, "%.1f", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("Free Look Look Sensitivity", &myFreeFlyLookSensitivity, 0.0001f, 0.05f, "%.4f", ImGuiSliderFlags_Logarithmic);
    ImGui::Text("Current Free Look: speed %.1f  sensitivity %.4f", myFreeFlyMoveSpeed, myFreeFlyLookSensitivity);

    ImGui::SeparatorText("Lighting");

    bool& enableDirectionalLight = GameDebugSettings::EnableDirectionalLight();
    bool& enableAmbientLight = GameDebugSettings::EnableAmbientLight();
    ImGui::Checkbox("Enable Directional Light", &enableDirectionalLight);
    ImGui::SameLine();
    ImGui::Checkbox("Enable Ambient Light", &enableAmbientLight);

    if (ImGui::Button("Reset Lighting to Defaults"))
    {
        GameDebugSettings::ResetLightingSettingsToDefaults();
    }

    float& directionalYawDegrees = GameDebugSettings::DirectionalLightYawDegrees();
    float& directionalPitchDegrees = GameDebugSettings::DirectionalLightPitchDegrees();
    float& directionalRollDegrees = GameDebugSettings::DirectionalLightRollDegrees();
    ImGui::SliderFloat("Directional Yaw (deg)", &directionalYawDegrees, -180.0f, 180.0f, "%.1f");
    ImGui::SliderFloat("Directional Pitch (deg)", &directionalPitchDegrees, -89.0f, 89.0f, "%.1f");
    ImGui::SliderFloat("Directional Roll (deg)", &directionalRollDegrees, -180.0f, 180.0f, "%.1f");

    float& directionalColorR = GameDebugSettings::DirectionalLightColorR();
    float& directionalColorG = GameDebugSettings::DirectionalLightColorG();
    float& directionalColorB = GameDebugSettings::DirectionalLightColorB();
    float directionalColor[3] = { directionalColorR, directionalColorG, directionalColorB };
    if (ImGui::ColorEdit3("Directional Color", directionalColor, ImGuiColorEditFlags_HDR))
    {
        directionalColorR = directionalColor[0];
        directionalColorG = directionalColor[1];
        directionalColorB = directionalColor[2];
    }

    float& directionalIntensity = GameDebugSettings::DirectionalLightIntensity();
    ImGui::DragFloat("Directional Intensity", &directionalIntensity, 0.01f, 0.0f, 100.0f, "%.3f");

    float& ambientColorR = GameDebugSettings::AmbientLightColorR();
    float& ambientColorG = GameDebugSettings::AmbientLightColorG();
    float& ambientColorB = GameDebugSettings::AmbientLightColorB();
    float ambientColor[3] = { ambientColorR, ambientColorG, ambientColorB };
    if (ImGui::ColorEdit3("Ambient Color", ambientColor, ImGuiColorEditFlags_HDR))
    {
        ambientColorR = ambientColor[0];
        ambientColorG = ambientColor[1];
        ambientColorB = ambientColor[2];
    }

    float& ambientIntensity = GameDebugSettings::AmbientLightIntensity();
    ImGui::DragFloat("Ambient Intensity", &ambientIntensity, 0.01f, 0.0f, 100.0f, "%.3f");

    ImGui::SeparatorText("Bounds");
    boundsChanged |= ImGui::Checkbox("Enable Camera Bounds", &myCameraBoundsEnabled);
    boundsChanged |= ImGui::InputFloat3("Camera Bounds Min", &myCameraBoundsMin.x, "%.1f");
    boundsChanged |= ImGui::InputFloat3("Camera Bounds Max", &myCameraBoundsMax.x, "%.1f");

    if (projectionChanged || boundsChanged)
    {
        myDebugCamFovRadians = std::clamp(myDebugCamFovRadians, kDebugCamFovMinRadians, kDebugCamFovMaxRadians);
        myFollowCamFovRadians = std::clamp(myFollowCamFovRadians, kDebugCamFovMinRadians, kDebugCamFovMaxRadians);
        myCameraFarPlane = std::clamp(myCameraFarPlane, 1.0f, kDefaultCameraFarPlane * 10.0f);
        myCameraNearPlane = std::clamp(myCameraNearPlane, 0.001f, std::max(0.002f, myCameraFarPlane - 0.001f));
        myCameraFarPlane = std::max(myCameraFarPlane, myCameraNearPlane + 0.001f);
        SanitizeCameraBounds();

        const float activeFovRadians = myDebugCameraEnabled ? myDebugCamFovRadians : myFollowCamFovRadians;
        const Tga::Vector2ui resolution = Tga::Engine::GetInstance()->GetRenderSize();
        ApplyPerspectiveToCameras(myCamera, myRenderCamera, activeFovRadians, myCameraNearPlane, myCameraFarPlane, resolution);
        myCamera.GetTransform().SetPosition(ClampCameraPositionToBounds(myCamera.GetTransform().GetPosition()));
    }
}
#endif
