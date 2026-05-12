# Animation Event System - Animator Guide

Animation events are named markers inside `.tgac` clip files. They let animation timing trigger gameplay, sound, and effects.

## What To Author

Add events to the clip's `events` list:

```json
{
  "animation_source_path": "animations\\FBX\\Player\\A_Player_Attack_Basic01.fbx",
  "start_time": 0.0,
  "end_time": 1.1,
  "is_looping": false,
  "events": [
    { "time": 0.25, "id": "attack_windup" },
    { "time": 0.52, "id": "attack_hit" },
    { "time": 0.90, "id": "attack_recover" }
  ]
}
```

Each event has:

- `time`: seconds inside the clip.
- `id`: the event name engineers listen for.

The event time must be between `start_time` and `end_time`.

## Naming

Use lower snake case names:

- `attack_hit`
- `attack_windup`
- `attack_recover`
- `footstep_left`
- `footstep_right`
- `cancel_start`
- `cancel_end`
- `vfx_slash`

Use names that describe the gameplay moment, not the implementation. For example, use `attack_hit`, not `enable_box_collider`.

## Timing Guidelines

- Put damage events on the frame where the weapon/contact should become active.
- Use start/end pairs for windows that last over time, such as cancel windows or invulnerability windows.
- Looping clips can fire the same event every loop. This is useful for footsteps.
- Non-looping clips fire events when playback crosses the marker.
- Keep important events slightly inside the clip range, not exactly on the first or last frame, unless engineers confirm that edge timing is required.

## Handoff Checklist

Before handing a clip to engineering:

1. Confirm the `.tgac` file points to the correct animation source.
2. Confirm `start_time` and `end_time` match the intended clip segment.
3. Add event ids using the agreed names.
4. Verify event times are inside the clip range.
5. Tell engineering which events are one-shot moments and which are window start/end pairs.

## Common Examples

Attack clip:

```json
"events": [
  { "time": 0.20, "id": "attack_windup" },
  { "time": 0.45, "id": "attack_hit" },
  { "time": 0.70, "id": "cancel_start" },
  { "time": 0.95, "id": "cancel_end" },
  { "time": 1.05, "id": "attack_recover" }
]
```

Walk or run loop:

```json
"events": [
  { "time": 0.18, "id": "footstep_left" },
  { "time": 0.62, "id": "footstep_right" }
]
```
