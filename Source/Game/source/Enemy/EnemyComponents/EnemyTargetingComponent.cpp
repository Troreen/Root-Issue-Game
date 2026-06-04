#include "EnemyTargetingComponent.h"

#include "Essentials.h"
#include "GameObject.h"

EnemyTargetingComponent::EnemyTargetingComponent(float aDetectionRange)
{
    myDetectionRange = aDetectionRange;
}

EnemyTargetingComponent::~EnemyTargetingComponent()
{
}

void EnemyTargetingComponent::OnUpdate(float)
{
    myTarget = Essentials::GetPlayer();

    if (!myTarget)
    {
        myTargetIsInRange = false;
        return;
    }

    auto& playerPos = myTarget->GetTransform().GetPosition();

    auto& ownerPos = GetOwner()->GetTransform().GetPosition();

    Vector3f diff = playerPos - ownerPos;

    float distance = diff.Length();

    myDistanceToTarget = distance;

    myTargetIsInRange = distance <= myDetectionRange;
}

bool EnemyTargetingComponent::IsTargetInRange() const
{
    return myTargetIsInRange;
}

GameObject* EnemyTargetingComponent::GetTarget() const
{
    return myTarget;
}

float EnemyTargetingComponent::GetDistanceToTarget() const
{
    return myDistanceToTarget;
}

CommonUtilities::Vector3<float> EnemyTargetingComponent::GetDirectionToTarget() const
{
    if (!myTarget)
    {
        return {};
    }

    auto direction = myTarget->GetTransform().GetPosition() - GetOwner()->GetTransform().GetPosition();

    direction.Normalize();

    return direction;
}