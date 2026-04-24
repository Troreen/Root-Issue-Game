# Collision System Handoff Summary

Last updated: 2026-04-24

## Current Architecture

- `RuntimeCollisionSystem` is the authoritative runtime collision path.
- Collision runs only from the StateStack `InGame` state:
  - `InGame::Update()` updates objects first.
  - Then `myRuntimeCollisionSystem.Run(myGameObjects)` executes.
  - Then `InGame::ConsumeCollisionContacts(...)` consumes contact events.
- `GameWorld` no longer runs collision against its own object list.
- Legacy `CollisionSystem`, `CollisionHandler`, and `ColliderComponent` code still exists but is dormant for the new runtime path.

## Implemented Runtime Behavior

- Authored colliders are created in `GameObjectFactoryRegistrations` from `.tgo` properties:
  - `colliderType`
  - `colliderSize`
  - `colliderOffset`
  - `colliderRadius`
  - `colliderIsTrigger`
- `BoxColliderComponent` and `SphereColliderComponent` update the owning `GameObject` hitbox.
- Collision currently uses AABB-vs-AABB checks through `GameObject::GetHitbox()`.
- Stable per-object collision IDs are assigned by `GameObject` and used for pair keys instead of pointer-address hashing.
- Contact events are emitted as `Enter`, `Stay`, and `Exit`.
- Exit contacts are skipped if either object no longer has a live runtime collider.

## Current Rules

Blocking pairs:

- `Player <-> WorldStatic`
- `BasicMeleeEnemy <-> WorldStatic`
- `Player <-> BasicMeleeEnemy`

Trigger pairs:

- `Player <-> Trigger`
- `Player <-> Pickup`

All other layer pairs currently default to `Ignore`.

## Resolution Model

- Blocking resolution is XZ-only for top-down gameplay.
- No Y-axis correction is applied.
- The minimum-penetration X/Z axis is selected.
- Resolution is iterative with `kMaxCollisionIterations = 4`.
- World-static resolution moves only the dynamic object.
- Player/enemy body overlap moves the player and leaves the enemy authoritative.
- Body collision does not apply damage.

## Contact Consumption

- `InGame::ConsumeCollisionContacts(...)` handles current interaction consumption.
- Trigger collider hooks:
  - `Enter` calls `OnTriggerEnter()`.
  - `Exit` calls `OnTriggerExit()`.
  - `Stay` does not re-enter.
- Player-pickup contacts remove and deactivate the pickup object.
- Pickup collection no longer relies on `PickUpComponent::OnUpdate()` self-polling as the authoritative path.

## Debug Drawing

- Debug ImGui exposes `Show Collision Shapes`.
- The toggle uses `GameDebugSettings::ShowColliderDebugLines()`.
- Box and sphere collider components draw their debug shapes when enabled.
- Box debug drawing now refreshes the AABB before rendering and draws from the same `GameObject::GetHitbox()` data used by collision.
- Box debug markers:
  - Yellow marker = collider center.
  - Orange marker = object origin.

## Known Bugs / Risks

- `test_static.tgo` has `colliderIsTrigger` authored as `Float3` instead of `Bool`. Runtime currently falls back to `false`, but the data is malformed and should be fixed.
- Collider debug boxes can look offset by half size when the mesh/model origin is not centered. For `test_static`, the collider is `200x200x200` with zero offset, so the centered collider extends 100 units from object origin on each axis.
- Runtime collision ignores object scale and rotation. Authored collider sizes are treated as world-space axis-aligned volumes.
- Sphere colliders participate in runtime collision through their owner AABB hitbox, not true sphere-vs-shape tests.
- Moving trigger box colliders only update every frame if `colliderConstantUpdate` is true or the runtime collision refresh path touches them. Static triggers are fine; dynamic trigger behavior needs verification.
- Removed/deactivated objects do not emit reliable trigger-exit cleanup because exit contacts are skipped when an object is no longer live.
- Pickup contact consumption currently removes/deactivates the object directly. It does not yet route through pickup rewards, audio, VFX, counters, or ranking systems.
- Trigger/interaction handling is centralized only for player-trigger and player-pickup pairs. Other trigger use cases need explicit rules.
- No broadphase exists. Cost grows with the number of active colliders in the current layer buckets.
- Legacy collision files are still compiled and may confuse future readers, even though the new runtime source of truth is `RuntimeCollisionSystem`.

## Not Implemented Yet

- Combat hitbox/hurtbox spawning and lifetime.
- Damage through attack hitboxes.
- Full collision rule matrix.
- Spatial broadphase.
- Persistent/serialized collision IDs.
- Full migration/deletion of legacy collision modules.

## Recommended Next Steps

1. Fix malformed collider authoring in `test_static.tgo` and audit other `.tgo` collider property types.
2. Decide whether collider dimensions should respect scene scale; if yes, apply scale consistently in collider AABB generation and debug drawing.
3. Add a proper pickup interaction component/API so collision contacts trigger rewards, VFX, audio, and counters instead of directly removing components.
4. Add explicit trigger-exit cleanup for objects removed while inside a trigger.
5. Implement combat hitbox/hurtbox collision as a separate phase from body blocking.
6. Add broadphase only if scene collider counts make the current filtered pair loops too expensive.
7. Deprecate or remove legacy collision modules once runtime parity is confirmed.

