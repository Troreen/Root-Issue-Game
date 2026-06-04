#pragma once
#include "ScriptComponent.h"
#include <Vector.hpp>

class GameObject;

class EnemyTargetingComponent : public ScriptComponent
{
public:

    EnemyTargetingComponent(float aDetectionRange);
    ~EnemyTargetingComponent();

    void OnUpdate(float aDeltaTime) override;

    bool IsTargetInRange() const;

    GameObject* GetTarget() const;

    float GetDistanceToTarget() const;

    CommonUtilities::Vector3<float> GetDirectionToTarget() const;

private:

    float myDetectionRange = 1000.0f;

    bool myTargetIsInRange = false;

    float myDistanceToTarget = 0.0f;

    GameObject* myTarget = nullptr;
};