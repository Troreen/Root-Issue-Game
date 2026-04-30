# Runtime Collision System Handoff

This document explains the current game collision system for agents working on gameplay, objects, triggers, pickups, enemies, or editor/object-definition tooling.

## Main Files

- `Source/Game/source/RuntimeCollisionSystem.h/.cpp`
  Runtime collision pass, broadphase collection, layer rules, shape narrow-phase checks, movement resolution, trigger contact generation, and debug logs.
- `Source/Game/source/CollisionTypes.h`
  Shared `CollisionRule`, `CollisionPhase`, and `CollisionContact`.
- `Source/Game/source/CollisionLayerRules.h`
  Small table used by runtime code to decide whether layer pairs block, trigger, or ignore.
- `Source/Game/source/CollisionShapeType.h`
  Numeric authored shape enum: `0=Box`, `1=Sphere`, `2=Capsule`, `3=OBB`.
- `Source/Game/source/*ColliderComponent.*`
  Shape components. Current authored components are:
  - `BoxColliderComponent`
  - `SphereColliderComponent`
  - `CapsuleColliderComponent`
  - `ObbColliderComponent`
- `Source/Game/source/GameObjectFactoryRegistrations.cpp`
  Reads `.tgo` collision properties and attaches the correct collider component.
- `Source/Game/source/StateStack/GameState_InGame.cpp`
  Calls `myRuntimeCollisionSystem.Run(myGameObjects)` each frame and consumes contacts for trigger enter/exit and pickups.
- `Source/Game/source/CameraSystem.cpp`
  ImGui collision debug controls.

Legacy `CollisionSystem` and `CollisionHandler` files still exist, but the active path is `RuntimeCollisionSystem`.

## Authored Object Properties

Collision is authored in `.tgo` files through properties. Objects without collision properties do not get a runtime collider, and that is allowed unless their layer requires one.

Common properties:

- `colliderType` int:
  - `0`: Box/AABB
  - `1`: Sphere
  - `2`: Capsule
  - `3`: OBB
- `colliderSize` `Vector3`
  Used by boxes and OBBs directly. Also used as a fallback for sphere/capsule dimensions if radius/height are missing.
- `colliderOffset` `Vector3`
  Offset applied to the authored collider, independent of model bounds.
- `colliderIsTrigger` bool
  When true, the collider should generate trigger contacts and not block movement.
- `colliderPivotBottomMiddle` bool
  Controls object-origin anchoring for code-authored colliders:
  - `true`: origin is bottom-middle, meaning X/Z are centered on the object and Y starts at bottom.
  - `false`: origin is bottom-left/corner style, meaning the collider extends in positive X, positive Y, and positive Z from the object origin.
- `colliderRadius` float
  Optional explicit sphere/capsule radius.
- `colliderHeight` float
  Optional explicit capsule full height.
- `colliderConstantUpdate` bool
  Box-only option used by existing code. Non-box colliders update every frame.

Factory behavior:

- If `colliderType` is missing or invalid, no collider is attached.
- Box and OBB require positive `colliderSize`.
- Sphere uses `colliderRadius` if present. If missing, it derives radius from `colliderSize`.
- Capsule uses `colliderRadius`/`colliderHeight` if present. If missing, it derives radius/height from `colliderSize`.

## Runtime Flow

`RuntimeCollisionSystem::Run(...)` is called once per frame from `InGame::Update`.

The runtime:

1. Collects active objects that have a supported collider component.
2. Buckets them by `ObjectLayer`:
   - `Player`
   - `BasicMeleeEnemy`
   - `WorldStatic`
   - `Trigger`
   - `Pickup`
3. Refreshes collider components so their shape data and broadphase AABBs are current.
4. Resolves blocking pairs.
5. Registers trigger pairs.
6. Emits `CollisionContact` entries with `Enter`, `Stay`, or `Exit`.
7. Stores current pairs for next-frame phase tracking.

Current hardcoded layer rules:

- `Player` vs `WorldStatic`: block
- `BasicMeleeEnemy` vs `WorldStatic`: block
- `Player` vs `BasicMeleeEnemy`: block
- `Player` vs `Trigger`: trigger
- `Player` vs `Pickup`: trigger

Important: `colliderIsTrigger=true` overrides blocking behavior in block pair loops. For example, a `WorldStatic` object with `colliderIsTrigger=true` should register trigger contact with the player instead of applying separation.

## Shape Model

Hitbox data is owned by collider components, not by `GameObject`. `GameObject` only provides transform, identity, layer, and component lookup. If code needs collision bounds, it should ask the relevant collider component for its AABB or shape data.

Every collider maintains an AABB. That AABB is used as broadphase bounds and for debug descriptions, not as the only source of truth.

The runtime then builds an internal `CollisionShape` from the actual component:

- Box: center/half extents from AABB, axis-aligned axes.
- Sphere: center/radius from `SphereColliderComponent`.
- Capsule: vertical segment plus radius.
- OBB: center, half extents, and owner transform axes from `ObbColliderComponent`.

Narrow-phase checks currently cover:

- Box vs Box
- Sphere vs Sphere
- Sphere vs Box
- Capsule vs Box
- Capsule vs Sphere
- Capsule vs Capsule
- OBB vs OBB
- OBB vs Box
- OBB vs Sphere
- OBB vs Capsule

Blocking resolution applies the returned separation vector to the dynamic object, then refreshes that object's collider.

## Triggers And Contacts

`CollisionContact` contains:

- `first`
- `second`
- `normal`
- `penetration`
- `phase`: `Enter`, `Stay`, or `Exit`

Blocking pairs can also create contacts after movement resolution. Trigger pairs create contacts without translating either object.

`GameState_InGame.cpp` consumes contacts and dispatches trigger phase events to collider components. Pickup deactivation also happens there.

If adding a new gameplay system:

- Read `myRuntimeCollisionSystem.GetContacts()` after `Run(...)`.
- Filter by layer, component type, or object name as needed.
- Respect contact phase. Use `Enter` for one-shot events and `Stay` for continuous overlap behavior.

## Debugging

Collision debug controls are in the camera ImGui panel:

- `Show Collision Shapes`
  Draws collider debug lines.
- `Log Collision Checks`
  Enables runtime collision logs.
- `Log Collision Non-Hits`
  Logs miss checks too; can be noisy.
- `Log Collision Resolution Details`
  Logs repeated stay/resolution details.
- `Collision Log Cap / Frame`
  Caps runtime collision logs per frame.
- `Log Collider Drawers`
  Logs drawer-side collider info.
- `Collider Drawer Log Cap / Frame`
  Caps drawer logs per frame.

Debug draw colors are component-specific. Trigger colliders generally draw magenta.

Useful log categories:

- `[CollisionDebug]`
  Runtime collection, block/trigger hit/miss, separation, normal, penetration, contact phase.
- `[ColliderDrawerDebug]`
  Render-side shape position/size information.
- `[CollisionAudit]`
  Missing collider warnings for layers expected to collide.

## Common Pitfalls

- Layer rules and collider trigger flags are separate.
  A `WorldStatic` object can still be non-blocking if its collider has `colliderIsTrigger=true`.
- Collider AABBs are still AABBs even for sphere/capsule/OBB.
  Do not assume the component AABB represents the final narrow-phase shape.
- Debug lines should draw from component-authored collider data, not model bounds.
  Model bounds are intentionally not part of collider authoring.
- Objects can legitimately have no collider.
  Only `WorldStatic`, `Player`, and `BasicMeleeEnemy` are audited as requiring colliders.
- `Local/*.vcxproj` is gitignored.
  If adding new `.cpp/.h` files, make sure local project files include them for local builds, but do not expect those project changes to be committed unless repo policy changes.
- The full Debug solution may fail on `EngineTests_Debug.exp`.
  `Game_Debug.lib` and `GameMain_Debug.exe` can still build successfully; check the build output before assuming game code failed.

## Extending The System

When adding a new collider shape:

1. Add the enum value to `CollisionShapeType.h`.
2. Add a component that:
   - Stores authored dimensions and offset.
   - Updates its own shape data.
   - Maintains its own AABB accessor for broadphase/debug use.
   - Draws debug lines from actual collider data.
   - Exposes `IsTrigger()`.
3. Include it in `GameObjectFactoryRegistrations.cpp`.
4. Include it in runtime:
   - `HasRuntimeCollider`
   - `HasTriggerCollider`
   - `RefreshRuntimeCollider`
   - `GetCollisionShape`
   - `GetColliderTypeName`
   - `TryComputeShapeSeparation`
5. Include trigger phase dispatch in `GameState_InGame.cpp`.
6. Add or update `.tgo` descriptions so authors know the numeric `colliderType`.
7. Build Debug and verify:
   - Shape appears with `Show Collision Shapes`.
   - `colliderIsTrigger=false` blocks where layer rules say block.
   - `colliderIsTrigger=true` does not block and emits contacts.
   - Debug logs show expected shape type, AABB, normal, penetration, and phase.
