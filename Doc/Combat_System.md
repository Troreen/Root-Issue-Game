# Combat System

This document explains the current combat-system slice from a gameplay/code point of view. It intentionally skips project setup and engine details.

## Purpose

The combat system owns temporary attack hitboxes and resolves combat overlaps separately from normal movement collision. Movement collision still handles blocking and trigger contacts. Combat overlap checks produce hit events and apply gameplay effects such as damage and knockback.

The current implemented slice supports player melee attacks against `BasicMeleeEnemy` objects.

## Main Types

### `CombatTeam`

`CombatTeam` identifies who owns an attack:

- `Neutral`
- `Player`
- `Enemy`

In the current implementation, team is stored in `AttackData` and `HitEvent`, but target filtering is done by `CombatTargetMask`, not by team. This leaves room for later team rules such as friendly fire, enemy-vs-player attacks, and neutral hazards.

### `AttackType`

`AttackType` describes the source/style of the hit:

- `MeleeLight`
- `MeleeCombo`
- `MeleeCharged`
- `DodgeAttack`
- `Ranged`
- `EnemyMelee`
- `EnemyRoll`

For v1, the player attack state starts `MeleeLight` attacks.

### `CombatTargetMask`

`CombatTargetMask` is a compact bit mask over `ObjectLayer`. It decides which object layers an attack is allowed to hit.

Example:

```cpp
attack.targetLayers.AddLayer(ObjectLayer::BasicMeleeEnemy);
```

The combat system checks this mask before doing narrow-phase overlap work. If a target's layer is not in the mask, it is ignored.

### `AttackData`

`AttackData` is the required input for spawning an attack.

Fields:

- `owner`: `GameObject*` that owns the attack. Required.
- `team`: combat team metadata for the attack.
- `type`: attack type metadata.
- `targetLayers`: object layers this attack can hit.
- `localCenterOffset`: hitbox center offset relative to the owner transform.
- `size`: transient box hitbox size.
- `activeDurationSeconds`: how long the attack remains active.
- `knockbackStrength`: magnitude of horizontal knockback.
- `damage`: damage sent to `DamageableComponent`.

`CombatSystem::StartAttack` rejects invalid attacks by returning `0` when:

- `owner == nullptr`
- `activeDurationSeconds <= 0.0f`
- `damage <= 0`

Valid attacks receive an increasing attack ID.

### `HitEvent`

`HitEvent` is emitted when an active attack successfully hits a target.

Fields:

- `attackId`
- `attacker`
- `target`
- `damage`
- `knockback`
- `type`

`CombatSystem::GetHitEventsThisFrame()` returns only the events from the most recent `CombatSystem::Update`. The list is cleared at the start of every combat update.

## Attack Lifetime

Attacks are transient data owned by `CombatSystem`. They are not spawned as temporary `GameObject`s.

Internally, each active attack stores:

- copied `AttackData`
- remaining active time
- attack ID
- a set of already-hit target collision IDs

The hit target set prevents repeated damage from the same swing while the target remains inside the hitbox. A new attack gets a new ID and a fresh hit set, so the same enemy can be hit again by the next swing.

When an attack's owner is missing or inactive, the attack is expired and removed.

## Hitbox Placement

Attack hitboxes are currently axis-aligned boxes created by `CollisionQuery::MakeBoxShape`.

The center is calculated from the owner transform:

```cpp
center =
    ownerPosition
    + ownerRight * localCenterOffset.x
    + worldUp * localCenterOffset.y
    + ownerForward * localCenterOffset.z;
```

This means:

- X offset follows the owner's right vector.
- Y offset is always world up.
- Z offset follows the owner's forward vector.

The hitbox size is the `AttackData::size` vector.

## Combat Update Flow

The gameplay update order in `InGame` is:

1. Game objects update.
2. `RuntimeCollisionSystem::Run(...)` resolves movement and trigger contacts.
3. Trigger contacts are dispatched.
4. `CombatSystem::Update(...)` resolves combat hitboxes.
5. VFX and audio update.

This keeps movement collision and combat collision separate. Combat hits do not create runtime collision contacts. They create `HitEvent`s and apply component-level gameplay effects.

## Hit Resolution

For each active attack, `CombatSystem::Update`:

1. Verifies the owner still exists and is active.
2. Builds a transient attack box from `AttackData`.
3. Iterates active game objects.
4. Skips the owner.
5. Skips objects outside `targetLayers`.
6. Skips targets already hit by this attack ID.
7. Skips targets without a runtime collider.
8. Refreshes the target collider shape.
9. Runs `CollisionQuery::TryComputeSeparation`.
10. On overlap, records the target collision ID as hit.
11. Applies damage if the target has `DamageableComponent`.
12. Applies knockback if the target has `KnockbackComponent`.
13. Emits a `HitEvent`.

The combat system logs attack starts and successful hits with `[Combat]` messages.

## Collision Query Use

`CollisionQuery` is a shared narrow-phase query helper. Combat uses it to compare transient attack shapes against real runtime collider shapes.

Combat currently creates attack boxes directly with:

```cpp
CollisionQuery::MakeBoxShape(center, size);
```

Targets are read from their actual runtime collider components through:

```cpp
CollisionQuery::GetShape(target);
```

The system can query box, sphere, capsule, and OBB-backed shapes through this shared shape representation. Combat only needs overlap information; it does not use the returned separation to move combat objects.

## Damage

Damage application is component-based.

If a target has `DamageableComponent`, combat calls:

```cpp
damageable->TakeDamage(attack.data.damage, attack.data.owner);
```

Targets without `DamageableComponent` can still produce hit events if they are valid combat targets, but they will not lose health.

## Knockback

Knockback is also component-based.

The combat system computes horizontal knockback from attacker position to target position:

1. `targetPosition - attackerPosition`
2. Y is forced to `0`
3. Direction is normalized
4. Direction is multiplied by `knockbackStrength`

If attacker and target are effectively on the same horizontal position, the attacker forward vector is used as fallback.

If the target has `KnockbackComponent`, combat calls:

```cpp
knockbackReceiver->ApplyImpulse(knockback);
```

`KnockbackComponent` stores velocity. During its normal object update, it translates the owner by `velocity * dt`, then damps the velocity over time. Because combat runs after runtime collision, newly applied knockback starts moving the target on the next frame's normal object update. Runtime collision then gets a chance to resolve world blocking after that movement.

## Player Melee Attack

`PlayerState_Attack` starts one attack when entering or restarting the attack swing.

Current v1 values:

```cpp
attack.owner = &player;
attack.team = CombatTeam::Player;
attack.type = AttackType::MeleeLight;
attack.damage = 25;
attack.localCenterOffset = CommonUtilities::Vector3<float>(0.0f, 90.0f, 115.0f);
attack.size = CommonUtilities::Vector3<float>(170.0f, 150.0f, 190.0f);
attack.activeDurationSeconds = 0.16f;
attack.knockbackStrength = 450.0f;
attack.targetLayers.AddLayer(ObjectLayer::BasicMeleeEnemy);
```

`PlayerState_Attack` uses `myHasSpawnedHitbox` to avoid repeatedly spawning attacks every frame while the same swing is active. When the player chains into another swing, `myHasSpawnedHitbox` is reset so the next swing can spawn a fresh attack and hit the same target again.

## Enemy Setup

`BuildBasicMeleeEnemy` configures enemies as combat targets:

- Uses `ObjectLayer::BasicMeleeEnemy`.
- Applies any authored collider.
- Adds a fallback capsule collider if no runtime collider exists.
- Adds `DamageableComponent`.
- Adds `KnockbackComponent`.

The fallback collider matters because combat queries require a runtime collider on the target.

## Services

`CombatService` is a lightweight static access point for starting attacks from gameplay code that does not own `InGame`.

`InGame` owns the actual `CombatSystem` instance and calls:

```cpp
CombatService::Set(&myCombatSystem);
```

`PlayerState_Attack` then starts attacks through:

```cpp
CombatService::StartAttack(attack);
```

If no combat system is registered, `CombatService::StartAttack` returns `0`.

## Adding a New Attack

To add a new attack:

1. Choose or add an `AttackType`.
2. Build an `AttackData`.
3. Set a valid owner.
4. Set damage and active duration.
5. Set local hitbox offset and size.
6. Add target layers to `targetLayers`.
7. Call `CombatService::StartAttack` or `CombatSystem::StartAttack`.

The target must have a runtime collider to be detected. It only receives damage or knockback if it has the matching components.

## Adding a New Combat Target

To make another object type hittable:

1. Put it on a layer that attacks target, or add its layer to attack target masks.
2. Give it a runtime collider.
3. Add `DamageableComponent` if it should take damage.
4. Add `KnockbackComponent` if it should be physically pushed.

Breakables can use the same path later once a real breakable component/factory exists.

## Current Limitations

- Combat attack volumes are transient boxes only.
- `CombatTeam` is metadata only; filtering is currently layer-based.
- Combat does not create trigger or runtime collision contacts.
- Hit events are stored for one frame only.
- Targets need runtime colliders to be detected.
- Knockback movement happens during the target's normal update after the hit frame.
- Player melee is the only attack wired in this slice.
