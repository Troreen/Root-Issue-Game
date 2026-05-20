#pragma once

#include <cstddef>
#include <deque>
#include <vector>

#include <tge/stringRegistry/StringRegistry.h>

struct AnimationEventRecord
{
    Tga::StringId id;
    Tga::StringId clipPath;
    Tga::StringId scriptId;
    float time = 0.0f;
};

// Per-animation-graph buffer. Pose generation writes events during Update;
// the dispatcher drains them later in the frame during LateUpdate.
class AnimationEventQueue
{
public:
    void Push(const AnimationEventRecord& anEvent);
    void Clear();

    bool IsEmpty() const;
    std::size_t Size() const;

    bool TryPop(AnimationEventRecord& outEvent);

    // Moves all currently queued events out in playback order.
    std::vector<AnimationEventRecord> Drain();

private:
    std::deque<AnimationEventRecord> myEvents;
};
