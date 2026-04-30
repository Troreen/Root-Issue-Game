#pragma once

#include "Component.h"

#include <CommonUtilities/AABB3D.hpp>
#include <CommonUtilities/Vector3.hpp>

using Vector3f = CommonUtilities::Vector3<float>;

class CapsuleColliderComponent final : public Component
{
public:
    explicit CapsuleColliderComponent(float aRadius = 1.0f,
                                      float aHeight = 2.0f,
                                      const Vector3f& aOffset = { 0.0f, 0.0f, 0.0f },
                                      bool anIsTrigger = false,
                                      bool aPivotBottomMiddle = true);

    bool OnTriggerEnter();
    bool OnTriggerExit();
    bool IsInside() const;

    void SetIsTrigger(bool anIsTrigger);
    bool IsTrigger() const;

    void Init(Tga::Engine& anEngine) override;
    void Update(float aDeltaTime) override;
    void Render() override;

    void SetRadius(float aRadius);
    float GetRadius() const;

    void SetHeight(float aHeight);
    float GetHeight() const;

    void SetOffset(const Vector3f& aOffset);
    const Vector3f& GetOffset() const;

    void SetPivotBottomMiddle(bool aPivotBottomMiddle);
    bool IsPivotBottomMiddle() const;

    Vector3f GetBottomCenter() const;
    Vector3f GetTopCenter() const;
    const CommonUtilities::AABB3D<float>& GetAABB() const;

private:
    void UpdateCapsule();

    bool myIsTrigger;
    bool myIsInside;
    bool myPivotBottomMiddle;

    float myRadius;
    float myHeight;
    Vector3f myOffset;
    Vector3f myBottomCenter;
    Vector3f myTopCenter;
    CommonUtilities::AABB3D<float> myAABB;
};
