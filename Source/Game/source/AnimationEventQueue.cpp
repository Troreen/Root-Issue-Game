#include "AnimationEventQueue.h"

void AnimationEventQueue::Push(const AnimationEventRecord& anEvent)
{
    myEvents.push_back(anEvent);
}

void AnimationEventQueue::Clear()
{
    myEvents.clear();
}

bool AnimationEventQueue::IsEmpty() const
{
    return myEvents.empty();
}

std::size_t AnimationEventQueue::Size() const
{
    return myEvents.size();
}

bool AnimationEventQueue::TryPop(AnimationEventRecord& outEvent)
{
    if (myEvents.empty())
    {
        return false;
    }

    outEvent = myEvents.front();
    myEvents.pop_front();
    return true;
}

std::vector<AnimationEventRecord> AnimationEventQueue::Drain()
{
    std::vector<AnimationEventRecord> drained;
    drained.reserve(myEvents.size());

    while (!myEvents.empty())
    {
        drained.push_back(myEvents.front());
        myEvents.pop_front();
    }

    return drained;
}
