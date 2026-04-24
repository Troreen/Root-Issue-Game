#pragma once

// LEGACY NOTE:
// This file is currently dormant and excluded from the active runtime build path.
// The authoritative runtime collision implementation now lives in RuntimeCollisionSystem.
// TODO: If this module is reactivated, mirror the layer-driven collision behavior from RuntimeCollisionSystem.

#include "ScriptComponent.h"
#include "AudioManager.h"
#include "CollectibleManager.h"
#include "RankSystem.h"

#include <CommonUtilities/AABB3D.hpp>
#include <CommonUtilities/Quaternion.hpp>
#include <CommonUtilities/Vector3.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class GameObject;
class HoverSystem;
class CameraSystem;
namespace CommonUtilities
{
    class InputHandler;
}

/// Handles runtime collision checks without keeping logic in GameWorld.
class CollisionSystemComponent final : public ScriptComponent
{
public:
    CollisionSystemComponent() = default;
    ~CollisionSystemComponent() override = default;

    void SetGameObjects(std::vector<std::unique_ptr<GameObject>>* aObjects);
    void SetPlayer(GameObject* aPlayer);
    void SetHoverSystem(HoverSystem* aHoverSystem);
    void SetCameraSystem(CameraSystem* aCameraSystem);
    void SetInputHandler(const CommonUtilities::InputHandler* anInputHandler);
    void ResetLevelRuntimeState();
    int GetSavedCoinsForLevel(const std::string& aScenePath);

protected:
    void OnUpdate(float aDeltaTime) override;

private:
    struct SchnozRespawnSnapshot
    {
        float speed = 0.0f;
        int defencePoints = 0;
        bool facingRight = false;
    };

    struct RuntimeObjectSnapshot
    {
        std::string tag;
        CommonUtilities::Vector3<float> position = { 0.0f, 0.0f, 0.0f };
        CommonUtilities::Quaternion<float> rotation;
        CommonUtilities::Vector3<float> scale = { 1.0f, 1.0f, 1.0f };
        CommonUtilities::AABB3D<float> hitbox;
        bool wasActive = true;
        bool hasSchnozState = false;
        SchnozRespawnSnapshot schnoz;
    };

    std::vector<std::unique_ptr<GameObject>>* myGameObjects = nullptr;
    GameObject* myPlayer = nullptr;
    HoverSystem* myHoverSystem = nullptr;
    CameraSystem* myCameraSystem = nullptr;
    const CommonUtilities::InputHandler* myInputHandler = nullptr;
    CollectibleManager myCollectibleManager;
    RankSystem* myRankSystem = nullptr;

    bool myHasRespawnPoint = false;
    CommonUtilities::Vector3<float> myLastCheckpointRespawnPosition = { 0.0f, 0.0f, 0.0f };
    bool myHaveAlreadyTriggerBloodSea = false;
    bool myRespawnFacingRight = true;

    std::unordered_map<std::string, int> mySavedCoinsByLevel;
    std::unordered_map<std::string, int> mySavedKeysByLevel;
    std::unordered_map<GameObject*, RuntimeObjectSnapshot> myRespawnSnapshots;
    bool myHasLoadedCoinProgress = false;
    bool myHasCapturedRespawnSnapshots = false;
    int myLastObservedCoinCount = 0;
    float myCollectibleRefreshTimer = 0.0f;

    void EnsureCoinProgressLoaded();
    void SaveCoinProgress() const;
    void RecordCurrentLevelCoinProgress();
    void CaptureInitialRespawnState();
    void ApplyRespawnResetPolicy(bool aIsCheckpointRespawn);
    void ResetAllPickupCounters();
};
