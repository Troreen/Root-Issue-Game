#pragma once

#include "Component.h"

#include <CommonUtilities/AABB3D.hpp>
#include <CommonUtilities/Vector3.hpp>

using Vector3f = CommonUtilities::Vector3<float>;

class ObbColliderComponent final : public Component
{
public:
    explicit ObbColliderComponent(const Vector3f& aSize = { 1.0f, 1.0f, 1.0f },
                                  const Vector3f& aOffset = { 0.0f, 0.0f, 0.0f },
                                  bool anIsTrigger = false,
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

    const Vector3f& GetCenter() const;
    const Vector3f* GetAxes() const;
    Vector3f GetHalfExtents() const;
    const CommonUtilities::AABB3D<float>& GetAABB() const;

private:
    void UpdateObb();

    bool myIsTrigger;
    bool myIsInside;
    bool myPivotBottomMiddle;

    Vector3f mySize;
    Vector3f myOffset;
    Vector3f myCenter;
    Vector3f myAxes[3];
    CommonUtilities::AABB3D<float> myAABB;
};
