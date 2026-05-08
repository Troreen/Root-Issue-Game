#include "EnemyAttackComponent.h"
#include "EnemyMovementComponent.h"
#include "GameObject.h"

EnemyAttackComponent::EnemyAttackComponent(const EnemyData& someEnemyData)
{
    myEnemyData = someEnemyData;
}

void EnemyAttackComponent::OnStart()
{
    myAttackData.owner = GetOwner();
    myMovement = GetOwner()->GetComponent<EnemyMovementComponent>();
    myAttackData.team = CombatTeam::Enemy;
    myAttackData.type = AttackType::EnemyMelee;
    myAttackData.collisionShape = CollisionShapeType::Sphere;
    myAttackData.damage = 1;
    myAttackData.localCenterOffset = CommonUtilities::Vector3<float>(0.0f, 90.0f, 0.0f);
    myAttackData.radius = 190.0f;
    myAttackData.activeDurationSeconds = 0.16f;
    myAttackData.knockbackStrength = 450.0f;
    myAttackData.onlyHitForwardHemisphere = true;
    myAttackData.targetLayers.AddLayer(ObjectLayer::Player);
}

void EnemyAttackComponent::OnUpdate(float aDeltaTime)
{
    if (myCooldownTimer > 0.0f)
    {
        myCooldownTimer -= aDeltaTime;
    }

    switch (myState)
    {
    case AttackState::Idle:
        break;

    case AttackState::Windup:
        UpdateWindup(aDeltaTime);
        break;

    case AttackState::Active:
        UpdateActive(aDeltaTime);
        break;

    case AttackState::Recovery:
        UpdateRecovery(aDeltaTime);
        break;
    }
}

bool EnemyAttackComponent::CanAttack() const
{
    return myState == AttackState::Idle && myCooldownTimer <= 0.0f;
}

void EnemyAttackComponent::UpdateWindup(float aDeltaTime)
{
    myTimer -= aDeltaTime;

    if (myMovement)
    {
        myMovement->RotateTowards(myAttackDirection, aDeltaTime);
        myMovement->StopMoving();
    }

    if (myTimer <= 0.0f)
    {
        myState = AttackState::Active;
        myTimer = myAttackData.activeDurationSeconds;
    }
}

void EnemyAttackComponent::UpdateActive(float aDeltaTime)
{
    if (!myHasAppliedAttack)
    {
        PerformAttack();
        myHasAppliedAttack = true;
    }

    if (myEnemyData.EnemyType == EnemyType::RollingEnemy && myMovement)
    {
        myMovement->MoveForward(aDeltaTime);
    }

    myTimer -= aDeltaTime;

    if (myTimer <= 0.0f)
    {
        myState = AttackState::Recovery;
        myTimer = myEnemyData.AttackRecovery;
    }
}

void EnemyAttackComponent::UpdateRecovery(float aDeltaTime)
{
    myTimer -= aDeltaTime;

    if (myTimer <= 0.0f)
    {
        myState = AttackState::Idle;
        myCooldownTimer = myEnemyData.AttackCooldown;
    }
}

void EnemyAttackComponent::PerformAttack()
{
    auto& transform = GetOwner()->GetTransform();

    float angle = std::atan2(myAttackDirection.x, myAttackDirection.z);
    auto rot = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(
        Vector3f::UnitY, angle);

    transform.SetRotation(rot);

    CombatService::StartAttack(myAttackData);
}

bool EnemyAttackComponent::IsAttacking() const
{
    return myState != AttackState::Idle;
}

void EnemyAttackComponent::StartAttack(GameObject* aTarget)
{
    if (!CanAttack() || !aTarget)
    {
        return;
    }

    auto& ownerPos = GetOwner()->GetTransform().GetPosition();
    auto& targetPos = aTarget->GetTransform().GetPosition();

    myAttackDirection = (targetPos - ownerPos);
    myAttackDirection.y = 0;
    myAttackDirection.Normalize();

    myState = AttackState::Windup;
    myTimer = myEnemyData.AttackWindup;

    myHasAppliedAttack = false;
}
