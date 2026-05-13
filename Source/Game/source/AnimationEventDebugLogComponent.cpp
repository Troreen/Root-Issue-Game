#include "AnimationEventDebugLogComponent.h"

#include "GameObject.h"

#include <iostream>

void AnimationEventDebugLogComponent::OnStart()
{
    const GameObject* owner = GetOwner();
    std::cout
        << "[AnimationEventDebug] Listening"
        << " owner=" << (owner ? owner->GetName() : "<no owner>")
        << std::endl;
}

void AnimationEventDebugLogComponent::OnAnimationEvent(const AnimationEventContext& anEvent)
{
    const char* ownerName = anEvent.owner ? anEvent.owner->GetName().c_str() : "<no owner>";

    std::cout
        << "[AnimationEventDebug] owner=" << ownerName
        << " event=" << anEvent.record.id.GetString()
        << " clip=" << anEvent.record.clipPath.GetString()
        << " time=" << anEvent.record.time
        << std::endl;
}
