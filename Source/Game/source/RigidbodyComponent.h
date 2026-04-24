#pragma once

#include "Component.h"

#include <CommonUtilities/Vector3.hpp>

class RigidbodyComponent final : public Component
{
public:
    enum class BodyType
    {
        Static,
        Kinematic,
        Dynamic
    };

    RigidbodyComponent();

    void SetBodyType(BodyType aType);
    BodyType GetBodyType() const;

    void SetMass(float aMass);
    float GetMass() const;

    void SetUseGravity(bool aUseGravity);
    bool GetUseGravity() const;

    void AddForce(const CommonUtilities::Vector3<float>& aForce);
    void AddImpulse(const CommonUtilities::Vector3<float>& anImpulse);
    void AddTorque(const CommonUtilities::Vector3<float>& aTorque);

    void SetLinearVelocity(const CommonUtilities::Vector3<float>& aVelocity);
    const CommonUtilities::Vector3<float>& GetLinearVelocity() const;

    void SetAngularVelocity(const CommonUtilities::Vector3<float>& aVelocity);
    const CommonUtilities::Vector3<float>& GetAngularVelocity() const;

private:
    BodyType myBodyType = BodyType::Dynamic;
    float myMass = 1.0f;
    bool myUseGravity = true;

    CommonUtilities::Vector3<float> myLinearVelocity = CommonUtilities::Vector3<float>::Zero;
    CommonUtilities::Vector3<float> myAngularVelocity = CommonUtilities::Vector3<float>::Zero;
};
