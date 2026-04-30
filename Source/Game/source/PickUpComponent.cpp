#include "PickUpComponent.h"
#include "BoxColliderComponent.h"
#include "CapsuleColliderComponent.h"
#include "ColliderComponent.h"
#include "Essentials/Essentials.h"
#include "ObbColliderComponent.h"
#include "SphereColliderComponent.h"

#include <CommonUtilities/AABB3D.hpp>
#include <iostream>

namespace
{
    bool TryGetColliderAabb(const GameObject& anObject, CommonUtilities::AABB3D<float>& outAabb)
    {
        if (const auto* box = anObject.GetComponent<BoxColliderComponent>())
        {
            outAabb = box->GetAABB();
            return true;
        }

        if (const auto* sphere = anObject.GetComponent<SphereColliderComponent>())
        {
            outAabb = sphere->GetAABB();
            return true;
        }

        if (const auto* capsule = anObject.GetComponent<CapsuleColliderComponent>())
        {
            outAabb = capsule->GetAABB();
            return true;
        }

        if (const auto* obb = anObject.GetComponent<ObbColliderComponent>())
        {
            outAabb = obb->GetAABB();
            return true;
        }

        if (const auto* collider = anObject.GetComponent<ColliderComponent>())
        {
            outAabb = collider->GetAabb();
            return true;
        }

        return false;
    }
}

void PickUpComponent::OnUpdate(float aDeltaTime)
{
    aDeltaTime;
}

bool PickUpComponent::IsTouching(const GameObject& aTarget)
{
    auto* owner = GetOwner();
    if (owner == nullptr)
        return false;

    CommonUtilities::AABB3D<float> a(Vector3f::Zero, Vector3f::Zero);
    CommonUtilities::AABB3D<float> b(Vector3f::Zero, Vector3f::Zero);
    if (!TryGetColliderAabb(*owner, a) || !TryGetColliderAabb(aTarget, b))
    {
        return false;
    }

    const auto minA = a.GetMin();
    const auto maxA = a.GetMax();
    const auto minB = b.GetMin();
    const auto maxB = b.GetMax();

    bool overlapX = (minA.x <= maxB.x + 20.f) && (maxA.x >= minB.x - 20.0f);
    //bool overlapY = (minA.y <= maxB.y + 20.f) && (maxA.y >= minB.y - 20.0f);
    bool overlapZ = (minA.z <= maxB.z + 20.f) && (maxA.z >= minB.z - 20.f);

    return overlapX && overlapZ;
}
