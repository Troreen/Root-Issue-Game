#pragma once

#include <cstddef>
#include <deque>
#include <vector>

#include <tge/stringRegistry/StringRegistry.h>

struct AnimationEventRecord
{
    Tga::StringId id;
    Tga::StringId clipPath;
    float time = 0.0f;
};

class AnimationEventQueue
{
public:
    void Push(const AnimationEventRecord& anEvent);
    void Clear();

    bool IsEmpty() const;
    std::size_t Size() const;

    bool TryPop(AnimationEventRecord& outEvent);
    std::vector<AnimationEventRecord> Drain();

private:
    std::deque<AnimationEventRecord> myEvents;
};
