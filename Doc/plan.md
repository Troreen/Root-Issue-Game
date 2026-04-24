# Node Script Animation Tree - Full Implementation Plan

## 1. Context And Assumptions

This plan targets the main project root and is intentionally designed for this starting state:

- No animation implementation logic in `GameWorld` should be required.
- The working directory may contain no usable `.tgac` or `.tgscript` assets.
- New animation states and transitions must be authorable without writing new C++ state code.
- Blending support is required.
- The common parts must be reusable across projects.

## 2. Primary Goals

1. Introduce a reusable, data-driven animation tree runtime that runs as a component, not from `GameWorld`.
2. Support state transitions and blend trees through script/node graphs.
3. Add a bootstrap path so teams can start from zero animation assets.
4. Keep rendering responsibilities separate from state logic.
5. Make cross-project adoption straightforward via shared module boundaries and conventions.

## 3. Non-Goals For V1

- Full IK solver stack.
- Complex layered masks beyond one basic additive/layer channel.
- New editor UX rewrite.
- Replacing existing direct clip playback everywhere on day one.

## 4. High-Level Architecture

### 4.1 Runtime Ownership

Animation behavior is moved from `GameWorld` into a dedicated runtime/component layer:

- `AnimationGraphComponent` (new): a `ScriptComponent`-style component that owns runtime state and updates every frame.
- `AnimationGraphRuntime` (new): execution engine wrapper over script runtime + pose extraction + sync time + event stream.
- `AnimationParameterStore` (new): strongly typed wrapper over dynamic/static properties.
- `AnimationEventQueue` (new): per-instance emitted animation events for gameplay consumption.

### 4.2 Rendering Boundary

- `AnimatedMeshComponent` remains responsible for model instance setup and draw.
- Animation graph runtime produces pose data.
- `AnimationGraphComponent` applies resulting pose to `AnimatedMeshComponent`.

### 4.3 Data Boundary

- Object definitions (`.tgo`) own references and default parameters.
- Graph assets (`.tgscript`) define transitions and blending.
- Clip assets (`.tgac`) define clip playback metadata and event markers.

## 5. Zero-Asset Bootstrap Strategy (Critical)

Because the starting directory may have no `.tgac` and no `.tgscript`, V1 must include a bootstrap path:

1. Create generator utilities to produce initial assets from model/animation source.
2. Add one code-driven fallback graph template for startup safety.
3. Automatically create required properties when missing.

### 5.1 Bootstrap Deliverables

- `AnimationClipBootstrapper` utility:
  - Input: source animation path + optional ranges.
  - Output: generated `.tgac` with sensible defaults.
- `AnimationGraphTemplateWriter` utility:
  - Generates starter `.tgscript` templates:
    - `idle_only`
    - `locomotion_blend_1d`
    - `state_machine_basic`
- Optional command path in tools/build scripts to generate missing assets.

## 6. Component-First Integration (No GameWorld Dependency)

### 6.1 Object Definition Contract

New/standardized object properties in `.tgo`:

- `model` (existing pattern)
- `animation_graph` (StringId path to graph)
- `clip_*` properties for clips consumed by graph
- dynamic parameters (examples):
  - `anim_speed`
  - `anim_direction`
  - `anim_is_grounded`
  - `anim_state`

### 6.2 Construction Path

Use current scene import + factory flow to attach animation behavior:

- Parse properties in scene import path.
- Build object via `GameObjectFactory`.
- Add `AnimatedMeshComponent` and `AnimationGraphComponent` from factory registration.

No logic should be added in `GameWorld` for per-state animation execution.

## 7. Planned File Changes

## 7.1 New Files (Game Layer)

- `Source/Game/source/AnimationGraphRuntime.h`
- `Source/Game/source/AnimationGraphRuntime.cpp`
- `Source/Game/source/AnimationGraphComponent.h`
- `Source/Game/source/AnimationGraphComponent.cpp`
- `Source/Game/source/AnimationEventQueue.h`
- `Source/Game/source/AnimationEventQueue.cpp`
- `Source/Game/source/AnimationAssetBootstrapper.h`
- `Source/Game/source/AnimationAssetBootstrapper.cpp`

## 7.2 Existing Files To Extend

- `Source/Game/source/AnimatedMeshComponent.h`
- `Source/Game/source/AnimatedMeshComponent.cpp`
- `Source/Game/source/GameObjectFactoryRegistrations.cpp`
- `Source/Game/source/SceneImportService.cpp`
- `Source/Game/source/ResolvedObjectDef.h` (if typed animation config wrappers are needed)

## 7.3 Optional Engine-Side Additions (If Needed For Reuse)

- Add/extend animation script nodes in:
  - `Source/Engine/tge/animation/Script/`
- Keep game-specific policy out of engine nodes.

## 8. Node Set For V1

Minimum node capabilities needed for script-authored states:

1. `Play Clip` (existing)
2. `Blend Pose` (existing)
3. `Adjust Speed` (existing)
4. `State Select` (new)
5. `Transition Blend` (new)
6. `Compare/Threshold` nodes (existing common nodes can cover many cases)
7. `Emit Event` or marker output bridge (new)

If `State Select` and `Transition Blend` are not added immediately, V1 can start with graph patterns built from existing common flow/property nodes, but dedicated nodes are preferred for maintainability.

## 9. Event Marker Plan (.tgac)

### 9.1 Data Extension

Extend `.tgac` format with marker list:

- `events: [ { "time": 0.35, "id": "footstep_l" }, ... ]`

### 9.2 Runtime Behavior

- Detect event crossings when playback advances.
- Handle looping correctly.
- Push events into `AnimationEventQueue` once per crossing.

## 10. Root Motion Plan

1. During runtime update:
   - Evaluate desired playback/sync.
   - Generate root-motion delta.
   - Expose delta to movement/physics integration layer.
2. Do not hard-apply world movement inside animation runtime.
3. Keep policy outside runtime to preserve gameplay authority.

## 11. Migration Strategy

### Phase A - Foundations

- Add `AnimationGraphRuntime` + `AnimationGraphComponent` skeleton.
- Wire component update loop and pose application.
- Keep fallback to current `Animator` path when graph data missing.

### Phase B - Asset Bootstrap

- Implement bootstrap utilities for `.tgac` and starter `.tgscript` generation.
- Add missing-asset diagnostics and auto-generate option in dev builds.

### Phase C - State/Blend Runtime

- Add state transition logic in graph form.
- Add locomotion 1D blend graph template.
- Validate smooth blending with no hard snaps.

### Phase D - Events And Root Motion

- Add `.tgac` markers + queue emission.
- Add root motion output channel and consumer integration point.

### Phase E - Reuse Hardening

- Extract cross-project shared API surface.
- Document conventions and required properties.
- Validate integration in a second project consumer.

## 12. Acceptance Criteria

1. A character can run an animation graph without any custom `GameWorld` animation code.
2. A new state can be added by changing graph/data only.
3. Blend transitions are smooth and configurable.
4. Missing `.tgac`/`.tgscript` can be bootstrapped with provided tooling.
5. Events from `.tgac` markers are observable by gameplay systems.
6. Root motion deltas are produced and consumable externally.
7. Existing non-graph characters continue functioning.

## 13. Test Plan

### 13.1 Runtime Behavior Tests

- Graph loads and updates when object spawns.
- Parameter changes drive transition selection.
- Blend factor interpolation correctness.
- Invalid graph/clip reference fallback behavior.

### 13.2 Data Tests

- `.tgac` serialize/deserialize with events.
- `.tgscript` template generation validity.
- Missing property handling in object definitions.

### 13.3 Integration Tests

- Scene import creates animated objects through factory path.
- Component update order does not require `GameWorld` hooks.
- Rendering receives valid pose each frame.

## 14. Risks And Mitigations

1. Risk: Runtime still leaks logic into `GameWorld`.
- Mitigation: enforce component ownership and forbid per-object animation decisions in `GameWorld`.

2. Risk: No assets available blocks progress.
- Mitigation: bootstrap generators + fallback graph template.

3. Risk: Event duplication on loop boundaries.
- Mitigation: explicit crossing logic and loop-safe marker tests.

4. Risk: Refactor breaks existing animator usage.
- Mitigation: staged rollout with compatibility fallback.

5. Risk: Script node gaps for transitions.
- Mitigation: add minimal dedicated nodes early (State Select + Transition Blend).

## 15. Milestones And Deliverables

### Milestone 1 (Infrastructure)

Deliverables:
- `AnimationGraphRuntime` + `AnimationGraphComponent` integrated in object factory path.
- No `GameWorld` animation logic required.

### Milestone 2 (Bootstrap)

Deliverables:
- Asset bootstrap utilities for graph and clip generation.
- Starter templates generated successfully from clean directory state.

### Milestone 3 (State/Blend)

Deliverables:
- Script-authored transitions + locomotion blending working in scene.

### Milestone 4 (Events/Root Motion)

Deliverables:
- `.tgac` markers emitted at runtime.
- Root motion output integrated with movement consumers.

### Milestone 5 (Reuse Package)

Deliverables:
- Shared API + integration docs.
- Validated in at least one additional project consumer.

## 16. Implementation Order Recommendation

1. Create runtime/component skeleton.
2. Factory + scene import wiring.
3. Pose application path with fallback.
4. Bootstrap generators.
5. Transition/blend graph behavior.
6. Event markers.
7. Root motion.
8. Documentation and second-project validation.

## 17. Done Definition

This initiative is done when a project with no prior GameWorld animation code and no preexisting `.tgac`/`.tgscript` can:

- Generate starter animation assets,
- Spawn an animated object through normal scene import/factory flow,
- Run script-authored state transitions with blending,
- Emit animation events,
- Output root motion,
- And do all this without introducing per-object animation logic in `GameWorld`.
