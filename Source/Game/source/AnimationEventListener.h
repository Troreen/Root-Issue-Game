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

struct AnimationEventMessage
{
    AnimationEventRecord record;
    GameObject* senderObject = nullptr;
    AnimationGraphComponent* sourceGraph = nullptr;
};

class AnimationEventListener
{
public:
    virtual ~AnimationEventListener() = default;
    virtual void OnAnimationEvent(const AnimationEventContext& anEvent) = 0;
};
