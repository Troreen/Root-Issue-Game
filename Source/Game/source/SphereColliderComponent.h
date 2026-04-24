#pragma once

#include "Component.h"
#include <CommonUtilities/Sphere.hpp>
#include <CommonUtilities/Vector3.hpp>

using Vector3f = CommonUtilities::Vector3<float>;

/// Sphere collider component.
/// Maintains a CommonUtilities::Sphere, updates it every frame from the owner
/// transform, and debug-draws a wireframe circle in Render().
class SphereColliderComponent final : public Component
{
public:
    explicit SphereColliderComponent(float aRadius = 1.0f,
                                     const Vector3f& aOffset = { 0.0f, 0.0f, 0.0f },
                                     bool anIsTrigger = false);

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

    void SetOffset(const Vector3f& aOffset);
    const Vector3f& GetOffset() const;

    const CommonUtilities::Sphere<float>& GetSphere() const;

private:
    void UpdateSphere();

    bool myIsTrigger;
    bool myIsInside;

    float myRadius;
    Vector3f myOffset;
    CommonUtilities::Sphere<float> mySphere;
};
