#pragma once

#include "AnimationEventListener.h"
#include "ScriptComponent.h"

class AnimationEventDebugLogComponent final
    : public ScriptComponent
    , public AnimationEventListener
{
protected:
    // Prints once so a test scene can confirm the listener was attached.
    void OnStart() override;

public:
    void OnAnimationEvent(const AnimationEventContext& anEvent) override;
};
