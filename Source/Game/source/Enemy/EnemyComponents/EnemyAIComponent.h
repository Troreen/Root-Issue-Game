#pragma once

#include "ScriptComponent.h"
#include "EnemyData.h"
#include "Vector.hpp"
#include <string>
#include <random>

class EnemyMovementComponent;
class EnemyTargetingComponent;
class EnemyAnimationComponent;
class ParticleEmitterComponent;
class EnemyAttackComponent;
class DamageableComponent;
class SphereColliderComponent;

enum class EnemyState
{
    Spawn,
    Idle,
    Wander,
    React,
    Chasing,
    Attacking,
    Hurt,
    Stunned,
    Death,
    ReturnHome
};

class EnemyAIComponent : public ScriptComponent
{
public:

    EnemyAIComponent(const EnemyData& someEnemyData);
    virtual ~EnemyAIComponent() = default;

    virtual void Init(Tga::Engine& aEngine) override;

    virtual void OnUpdate(float aDeltaTime) override = 0;

    virtual void Reset() override;
    virtual void Save() override;

    void Spawn();

protected:

    void ChangeState(EnemyState aState);

    void PickNewDirection();

    void MoveTowardsHome(float aDeltaTime);

    float GetRandomFloat(float min, float max);
    float GetRandomAngleDegreeToRad(float min, float max);

    std::string StringifyState(const EnemyState& aState) const;

protected:

    EnemyData myEnemyData;

    EnemyState myCurrentState = EnemyState::Idle;
    EnemyState myPreviousState = EnemyState::Idle;

    EnemyMovementComponent* myMovement = nullptr;
    EnemyTargetingComponent* myTargeting = nullptr;
    EnemyAnimationComponent* myAnimation = nullptr;
    ParticleEmitterComponent* myEmitterComponent = nullptr;
    EnemyAttackComponent* myAttack = nullptr;
    DamageableComponent* myDamageableComponent = nullptr;
    //SphereColliderComponent* myCollider = nullptr;

    std::mt19937 myRandomEngine;

    bool myIsAggro = false;
    bool myHasBeenInitialized = false;
    bool myActiveAfterSave = true;
    bool myFirstHit = true;

    float mySpawnTimer = 0.f;
    float myIdleTimer = 0.f;
    float myWanderTimer = 0.f;
    float myReactionTimer = 0.8f;
    float myDeathTimer = 3.f;
    float myStunnedTimer = 1.0f;
    float myHurtTimer = 0.2f;
    const float myHurtDuration = 0.2f;

    float myChaseRange = 1000.f;
    float myLeashDistance = 1800.f;
    float myReturnTolerance = 50.f;

    CommonUtilities::Vector3<float> myWanderDirection;
    CommonUtilities::Vector3<float> myStartPosition;
};