#pragma once

#include "AnimationEventQueue.h"

class AnimationGraphComponent;
class GameObject;

struct AnimationEventContext
{
    AnimationEventRecord record;
    GameObject* owner = nullptr;
    AnimationGraphComponent* graph = nullptr;
};

// Payload used when animation events need to leave the owning GameObject.
// Component listeners are still the preferred path for object-local behavior.
struct AnimationEventMessage
{
    AnimationEventRecord record;
    GameObject* senderObject = nullptr;
    AnimationGraphComponent* sourceGraph = nullptr;
};

// Implement this on a Component to receive authored TGAC events from the
// AnimationEventDispatcherComponent attached to the same GameObject.
class AnimationEventListener
{
public:
    virtual ~AnimationEventListener() = default;
    virtual void OnAnimationEvent(const AnimationEventContext& anEvent) = 0;
};
