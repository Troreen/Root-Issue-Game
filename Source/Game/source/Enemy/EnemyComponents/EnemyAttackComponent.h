#pragma once
#include "ScriptComponent.h"
#include "CombatSystem.h"
#include "CollisionListener.h"
#include "EnemyData.h"


class EnemyMovementComponent;
class AnimationGraphComponent;

enum class AttackState
{
    Idle,
    Windup,
    Active,
    Recovery
};

class EnemyAttackComponent : public ScriptComponent, public CollisionListener
{
public:

    EnemyAttackComponent() = delete;
    EnemyAttackComponent(const EnemyData& someEnemyData);

    void OnStart() override;
    void OnUpdate(float aDeltaTime) override;
    void OnCollisionEnter(const CollisionContact& aContact, GameObject& anOther) override;
    void OnCollisionStay(const CollisionContact& aContact, GameObject& anOther) override;
    void OnCollisionExit(const CollisionContact& aContact, GameObject& anOther) override;

    void StartAttack(GameObject* aTarget);

    bool IsAttacking() const;
    bool CanAttack() const;

private:

    void UpdateWindup(float aDeltaTime);
    void UpdateActive(float aDeltaTime);
    void UpdateRecovery(float aDeltaTime);

    void PerformAttack();
    void FinishRollingAttack();
    void StopRollingAttackOnWorldCollision(GameObject& anOther);

private:

    EnemyMovementComponent* myMovement = nullptr;
    AnimationGraphComponent* myAnimationGraph = nullptr;

    EnemyData myEnemyData;
    AttackData myAttackData;

    AttackState myState = AttackState::Idle;

    float myTimer = 0.0f;
    float myCooldownTimer = 0.0f;

    CommonUtilities::Vector3<float> myAttackDirection;
    CommonUtilities::Vector3<float> myRollStartPosition;

    bool myHasAppliedAttack = false;
};

