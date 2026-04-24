#pragma once

#include "Component.h"

#include <CommonUtilities/Vector2.hpp>

#include <string>

class VfxEmitterComponent : public Component
{
public:
    struct Settings
    {
        std::string effectId;
        std::string space = "world";
        float spawnIntervalSeconds = 0.1f;
        int burstOnStartCount = 0;
        int burstOnActivateCount = 0;
        float sizeMultiplier = 1.0f;
        CommonUtilities::Vector2<float> screenPosition = { 0.0f, 0.0f };
    };

    explicit VfxEmitterComponent(Settings someSettings);

    void Init(Tga::Engine& anEngine) override;
    void Update(float aDeltaTime) override;
    void OnActiveChanged(bool isActive) override;

private:
    void EmitOnce();

    Settings mySettings;
    float myTimeUntilNextSpawn = 0.0f;
    bool myEmitActivationBurstNextUpdate = false;
};
