#pragma once

#include <CommonUtilities/Vector2.hpp>
#include <CommonUtilities/Vector3.hpp>
#include <tge/math/Color.h>

#include <string>
#include <unordered_map>
#include <vector>

#include <ParticlePool.h>

namespace Tga
{
    class Texture;
}

class VfxSystem
{
public:
    enum class ParticleMotionMode
    {
        Static,
        Falling,
        Floating
    };

    enum class Space
    {
        World,
        Screen
    };

    struct SpawnParams
    {
        CommonUtilities::Vector3<float> worldPosition = { 0.0f, 0.0f, 0.0f };
        CommonUtilities::Vector2<float> screenPosition = { 0.0f, 0.0f };
        float sizeMultiplier = 1.0f;
        float rotationRadians = 0.0f;
        float ownerForwardSign = 1.0f;
        bool useCustomColor = false;
        Tga::Color color = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    VfxSystem();

    bool Init();
    void Update(float aDeltaTime);
    void Render();

    bool SpawnEffect(const std::string& anEffectId, const SpawnParams& someParams = {});
    bool SpawnWorldEffect(const std::string& anEffectId, const CommonUtilities::Vector3<float>& aPosition, float aSizeMultiplier = 1.0f, float anOwnerForwardSign = 1.0f);
    bool SpawnScreenEffect(const std::string& anEffectId, const CommonUtilities::Vector2<float>& aPosition, float aSizeMultiplier = 1.0f);
    void BeginSceneTransition(const std::string& aFromScene, const std::string& aToScene);
    bool ReloadDefinitions();
    void ClearActiveEffects();

    bool SpawnPoolParticle(const ParticleType& aParticleType, const ParticleSettings& someSettings);

    int GetActiveCount() const;
    int GetCapacity() const;
    int GetDefinitionCount() const;
    std::vector<std::string> GetEffectIds() const;

private:
    struct EffectDefinition
    {
        std::string id;
        std::string texturePath;
        Space space = Space::World;
        int columns = 1;
        int rows = 1;
        int frameCount = 1;
        float fps = 24.0f;
        bool loop = false;
        bool debugLogFrames = false;
        float lifetimeSeconds = 0.0f;
        CommonUtilities::Vector2<float> size = { 100.0f, 100.0f };
        CommonUtilities::Vector2<float> pivot = { 0.5f, 0.5f };
        CommonUtilities::Vector3<float> spawnOffsetWorld = { 0.0f, 0.0f, 0.0f };
        CommonUtilities::Vector2<float> spawnOffsetScreen = { 0.0f, 0.0f };
        bool useOwnerForward = false;
        ParticleMotionMode motionMode = ParticleMotionMode::Static;
        CommonUtilities::Vector3<float> initialVelocity = { 0.0f, 0.0f, 0.0f };
        CommonUtilities::Vector3<float> acceleration = { 0.0f, 0.0f, 0.0f };
        float driftStrength = 0.0f;
        float driftFrequency = 0.0f;
        Tga::Color color = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    struct ActiveEffect
    {
        bool isActive = false;
        const EffectDefinition* definition = nullptr;
        const Tga::Texture* texture = nullptr;
        SpawnParams params;
        float ageSeconds = 0.0f;
        int lastDebugFrameIndex = -1;
        CommonUtilities::Vector3<float> runtimeWorldPosition = { 0.0f, 0.0f, 0.0f };
        CommonUtilities::Vector3<float> runtimeVelocity = { 0.0f, 0.0f, 0.0f };
        float driftPhase = 0.0f;
    };

    bool LoadEffectDefinitions();
    bool LoadDefinitionFromFile(const std::string& aPath);
    float CalculateDurationSeconds(const EffectDefinition& aDefinition) const;

    std::unordered_map<std::string, EffectDefinition> myDefinitions;
    std::unordered_map<ParticleType, ParticlePool> myParticlePools;
    std::vector<ActiveEffect> myActiveEffects;

    static constexpr int kMaxActiveEffects = 512;
};

class VfxService
{
public:
    static void Set(VfxSystem* aSystem);
    static VfxSystem* Get();

    static bool SpawnWorldEffect(const std::string& anEffectId, const CommonUtilities::Vector3<float>& aPosition, float aSizeMultiplier = 1.0f, float anOwnerForwardSign = 1.0f);
    static bool SpawnScreenEffect(const std::string& anEffectId, const CommonUtilities::Vector2<float>& aPosition, float aSizeMultiplier = 1.0f);

    static bool SpawnParticle(const ParticleType& aParticleType, const ParticleSettings& someSettings);

private:
    static VfxSystem* ourSystem;
};
