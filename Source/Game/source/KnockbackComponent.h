#pragma once

#include "ScriptComponent.h"

#include <CommonUtilities/Vector3.hpp>

class KnockbackComponent final : public ScriptComponent
{
public:
    void ApplyImpulse(const CommonUtilities::Vector3<float>& anImpulse);

protected:
    void OnUpdate(float aDeltaTime) override;

private:
    CommonUtilities::Vector3<float> myVelocity = { 0.0f, 0.0f, 0.0f };
    float myDampingPerSecond = 8.0f;
};
