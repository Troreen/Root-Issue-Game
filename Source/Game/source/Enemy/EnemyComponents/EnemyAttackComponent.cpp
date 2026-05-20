#include "EnemyAttackComponent.h"
#include "EnemyMovementComponent.h"
#include "AnimationGraphComponent.h"
#include "GameObject.h"

EnemyAttackComponent::EnemyAttackComponent(const EnemyData& someEnemyData)
{
    myEnemyData = someEnemyData;
}

void EnemyAttackComponent::OnStart()
{
    myAttackData.owner = GetOwner();
    myMovement = GetOwner()->GetComponent<EnemyMovementComponent>();
    myAnimationGraph = GetOwner()->GetComponent<AnimationGraphComponent>();
    
    switch (myEnemyData.EnemyType)
    {
    case EnemyType::BasicEnemy:
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
        break;
    case EnemyType::RollingEnemy:
        myAttackData.team = CombatTeam::Enemy;
        myAttackData.type = AttackType::EnemyRoll;
        myAttackData.collisionShape = CollisionShapeType::Sphere;
        myAttackData.damage = 1;
        myAttackData.localCenterOffset = CommonUtilities::Vector3<float>(0.0f, 90.0f, 0.0f);
        myAttackData.radius = 200.0f;
        myAttackData.activeDurationSeconds = 0.16f;
        myAttackData.knockbackStrength = 450.0f;
        myAttackData.onlyHitForwardHemisphere = false;
        myAttackData.targetLayers.AddLayer(ObjectLayer::Player);
            break;
    default:
        break;
    }

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
        myRollStartPosition = GetOwner()->GetTransform().GetPosition();
    }

    if (myTimer <= 0.0f)
    {
        myState = AttackState::Active;
        myTimer = myEnemyData.AttackWindup;
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
        if (myAnimationGraph)
        {
            myAnimationGraph->SetFloatParameter("w_roll_idle", 1.0f);
            myAnimationGraph->SetFloatParameter("w_charge", 0.0f);
        }

        myMovement->MoveForward(aDeltaTime);

        auto& pos = GetOwner()->GetTransform().GetPosition();

        float distance = (pos - myRollStartPosition).Length();

        if (distance >= myEnemyData.RollDistance)
        {
            //myState = AttackState::Recovery;
            myState = AttackState::Idle;

            myTimer = myEnemyData.AttackRecovery;
            myCooldownTimer = myEnemyData.AttackCooldown;

            myMovement->StopMoving();
            myAnimationGraph->SetFloatParameter("w_roll_idle", 0.0f);
            myAnimationGraph->SetFloatParameter("w_knockback", 1.0f);
        }
    }
    else
    {
        myTimer -= aDeltaTime;

        if (myTimer <= 0.0f)
        {
            //myState = AttackState::Recovery;
            myState = AttackState::Idle;

            myTimer = myEnemyData.AttackRecovery;

            myCooldownTimer = myEnemyData.AttackCooldown;
        }
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
