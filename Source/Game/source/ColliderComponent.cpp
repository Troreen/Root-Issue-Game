#include "ColliderComponent.h"

#include "GameObject.h"


ColliderComponent::ColliderComponent(const Vector3f& aSize, const Vector3f& aOffset)
    : mySize(aSize)
    , myOffset(aOffset)
{
}

void ColliderComponent::Init(Tga::Engine& /*anEngine*/)
{
    UpdateHitbox();
}

void ColliderComponent::Update(float /*aDeltaTime*/)
{
    UpdateHitbox();
}

void ColliderComponent::SetSize(const Vector3f& aSize)
{
    mySize = aSize;
    UpdateHitbox();
}

const Vector3f& ColliderComponent::GetSize() const
{
    return mySize;
}

void ColliderComponent::SetOffset(const Vector3f& aOffset)
{
    myOffset = aOffset;
    UpdateHitbox();
}

const Vector3f& ColliderComponent::GetOffset() const
{
    return myOffset;
}

const CommonUtilities::AABB3D<float>& ColliderComponent::GetAabb() const
{
    return myAabb;
}

void ColliderComponent::UpdateHitbox()
{
    auto* owner = GetOwner();
    if (!owner)
    {
        return;
    }

    const Vector3f pos = owner->GetTransform().GetPosition() + myOffset;
    const Vector3f half = mySize * 0.5f;

    myAabb = CommonUtilities::AABB3D<float>(pos - half, pos + half);
    owner->SetHitbox(myAabb);
}
