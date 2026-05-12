# Animation Event System - Engineering Guide

This guide explains how gameplay code observes animation events emitted from `.tgac` clips.

## Runtime Flow

Animation events are authored as markers in animation clip metadata:

```json
"events": [
  { "time": 0.52, "id": "attack_hit" }
]
```

At runtime the flow is:

1. `PlayClipNode` advances clip time and detects crossed event markers.
2. `AnimationGraphRuntime` converts crossed markers into `AnimationEventRecord`s.
3. `AnimationGraphComponent` stores those records in its `AnimationEventQueue`.
4. `AnimationEventDispatcherComponent` drains that queue once in `LateUpdate`.
5. The dispatcher notifies local `AnimationEventListener` components and forwards a global `MessageType::AnimationEvent` through `PostMaster`.

The dispatcher is automatically added by the object factory for objects with `animation_graph`.

## Local Listeners

Use local listeners for behavior owned by the animated object: hitboxes, combo windows, cancel windows, character VFX, and character-specific sounds.

```cpp
#include "AnimationEventListener.h"
#include "ScriptComponent.h"

class PlayerAttackWindowComponent final
    : public ScriptComponent
    , public AnimationEventListener
{
public:
    void OnAnimationEvent(const AnimationEventContext& anEvent) override
    {
        const std::string eventName = anEvent.record.id.GetString();

        if (eventName == "attack_hit")
        {
            // Enable or resolve this owner's attack hitbox.
        }
        else if (eventName == "cancel_start")
        {
            // Open this owner's cancel window.
        }
        else if (eventName == "cancel_end")
        {
            // Close this owner's cancel window.
        }
    }
};
```

Listener callbacks receive:

- `record.id`: event name, for example `attack_hit`.
- `record.clipPath`: source animation path recorded by the clip.
- `record.time`: authored event time in seconds.
- `owner`: the animated `GameObject`.
- `graph`: the source `AnimationGraphComponent`.

Disabled listener components are skipped.

## Global Messages

Use the global `PostMaster` bridge for systems that intentionally observe many objects or do not live on the animated object: debug overlays, analytics, global audio routing, mission scripting, or central combat telemetry.

Subscribe to `MessageType::AnimationEvent` and read `AnimationEventMessage` from `Message::myData`:

```cpp
void MySubscriber::Receive(const Message& aMessage)
{
    if (aMessage.myMessageType != MessageType::AnimationEvent)
    {
        return;
    }

    const auto* eventMessage = std::any_cast<AnimationEventMessage>(&aMessage.myData);
    if (!eventMessage)
    {
        return;
    }

    const std::string eventName = eventMessage->record.id.GetString();
}
```

## Rules

- Do not call `AnimationGraphComponent::GetEventQueue().Drain()` from normal gameplay code. The dispatcher owns draining so multiple observers can receive the same event.
- Prefer local listeners for object-owned gameplay. Use `PostMaster` only when the event is genuinely global.
- Keep event ids stable and data-driven. Do not add a new `MessageType` for every marker id.
- Author paired events for windows that open and close, for example `cancel_start` and `cancel_end`.

## Useful Files

- `Source/Game/source/AnimationEventListener.h`
- `Source/Game/source/AnimationEventDispatcherComponent.h`
- `Source/Game/source/AnimationEventQueue.h`
- `Source/Game/source/AnimationGraphComponent.h`
- `Source/Engine/tge/animation/Script/PlayClipNode.cpp`
