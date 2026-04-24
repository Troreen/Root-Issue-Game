#include "PickUpComponent.h"
#include "Essentials/Essentials.h"
#include <iostream>

void PickUpComponent::OnUpdate(float aDeltaTime)
{
    aDeltaTime;
}

bool PickUpComponent::IsTouching(const GameObject& aTarget)
{
    auto* owner = GetOwner();
    if (owner == nullptr)
        return false;

    const auto& a = owner->GetHitbox();
    const auto& b = aTarget.GetHitbox();

    const auto minA = a.GetMin() - owner->GetTransform().GetPosition();
    const auto maxA = a.GetMax() - owner->GetTransform().GetPosition();
    const auto minB = b.GetMin() - aTarget.GetTransform().GetPosition();
    const auto maxB = b.GetMax() - aTarget.GetTransform().GetPosition();

    bool overlapX = (minA.x <= maxB.x + 20.f) && (maxA.x >= minB.x - 20.0f);
    //bool overlapY = (minA.y <= maxB.y + 20.f) && (maxA.y >= minB.y - 20.0f);
    bool overlapZ = (minA.z <= maxB.z + 20.f) && (maxA.z >= minB.z - 20.f);

    return overlapX && overlapZ;
}
