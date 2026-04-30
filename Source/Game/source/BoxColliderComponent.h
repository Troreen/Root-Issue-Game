#pragma once

#include "Component.h"
#include <AABB3D.hpp>
#include <Vector3.hpp>

using Vector3f = CommonUtilities::Vector3<float>;

/// Axis-aligned box collider component.
/// Owns its authored AABB every frame and debug-draws the AABB edges.
class BoxColliderComponent final : public Component
{
public:
    explicit BoxColliderComponent(const Vector3f& aSize = { 1.0f, 1.0f, 1.0f },
                                  const Vector3f& aOffset = { 0.0f, 0.0f, 0.0f },
                                  bool anIsTrigger = false,
                                  bool aConstantUpdate = false,
                                  bool aPivotBottomMiddle = false);

    bool OnTriggerEnter();
    bool OnTriggerExit();
    bool IsInside() const;

    void SetIsTrigger(bool anIsTrigger);
    bool IsTrigger() const;

    void Init(Tga::Engine& anEngine) override;
    void Update(float aDeltaTime) override;
    void Render() override;

    void SetSize(const Vector3f& aSize);
    const Vector3f& GetSize() const;

    void SetOffset(const Vector3f& aOffset);
    const Vector3f& GetOffset() const;

    void SetPivotBottomMiddle(bool aPivotBottomMiddle);
    bool IsPivotBottomMiddle() const;

    const CommonUtilities::AABB3D<float>& GetAABB() const;

private:
    void UpdateAABB();

    bool myIsTrigger;
    bool myIsInside;
    bool myConstantUpdate;
    bool myPivotBottomMiddle;

    Vector3f mySize;
    Vector3f myOffset;
    CommonUtilities::AABB3D<float> myAABB;
};
