#pragma once
#include "ScriptComponent.h"
#include "CombatSystem.h"
#include "CollisionListener.h"
#include "EnemyData.h"


class EnemyMovementComponent;
class EnemyAnimationComponent;

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
    void CancelAttack();


    bool ConsumeWallHit();
    void FinishRollingAttack();
    bool IsRollingActive() const;

    bool DidRollHitPlayerThisFrame() const;
    bool DidRollHitWallThisFrame() const;


private:


    void UpdateWindup(float aDeltaTime);
    void UpdateActive(float aDeltaTime);
    void UpdateRecovery(float aDeltaTime);

    void PerformAttack();
    void StopRollingAttackOnWorldCollision(GameObject& anOther);



private:

    EnemyMovementComponent* myMovement = nullptr;
    EnemyAnimationComponent* myAnimation = nullptr;

    EnemyData myEnemyData;
    AttackData myAttackData;

    AttackState myState = AttackState::Idle;

    std::uint64_t myCombatAttackId = 0;

    float myTimer = 0.0f;
    float myCooldownTimer = 0.0f;

    CommonUtilities::Vector3<float> myAttackDirection;
    CommonUtilities::Vector3<float> myRollStartPosition;

    bool myHasAppliedAttack = false;
    bool myFinishedRollingAttack = false;
    bool myDidCollideWithWall = false;
};

