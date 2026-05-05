#include "KnockbackComponent.h"

#include "GameObject.h"

#include <algorithm>

void KnockbackComponent::ApplyImpulse(const CommonUtilities::Vector3<float>& anImpulse)
{
    myVelocity += anImpulse;
}

void KnockbackComponent::OnUpdate(float aDeltaTime)
{
    if (myVelocity.LengthSqr() <= 1.0f)
    {
        myVelocity = { 0.0f, 0.0f, 0.0f };
        return;
    }

    GetOwner()->GetTransform().Translate(myVelocity * aDeltaTime);

    const float damping = (std::max)(0.0f, 1.0f - myDampingPerSecond * aDeltaTime);
    myVelocity = myVelocity * damping;
}
