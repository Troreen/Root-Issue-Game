#pragma once

#include "Component.h"
#include <CommonUtilities/AABB3D.hpp>
#include <CommonUtilities/Vector3.hpp>

using Vector3f = CommonUtilities::Vector3<float>;

class ColliderComponent final : public Component
{
public:
    explicit ColliderComponent(const Vector3f& aSize = { 1.0f, 1.0f, 1.0f },
                               const Vector3f& aOffset = { 0.0f, 0.0f, 0.0f });

    void Init(Tga::Engine& anEngine) override;
    void Update(float aDeltaTime) override;

    void SetSize(const Vector3f& aSize);
    const Vector3f& GetSize() const;

    void SetOffset(const Vector3f& aOffset);
    const Vector3f& GetOffset() const;

    const CommonUtilities::AABB3D<float>& GetAabb() const;

private:
    void UpdateHitbox();

    Vector3f mySize;
    Vector3f myOffset;
    CommonUtilities::AABB3D<float> myAabb;
};
