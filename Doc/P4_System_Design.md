# Game Systems — Technical Reference

Isometric action-combat · TGE engine · 5 programmers

---

## Table of Contents

1. [Content pipeline](#1-content-pipeline)
2. [Factory types](#2-factory-types)
3. [Object layers](#3-object-layers)
4. [Static world objects](#4-static-world-objects)
5. [Auto-tiling](#5-auto-tiling)
6. [Player stack](#6-player-stack)
7. [Combat system](#7-combat-system)
8. [Enemies](#8-enemies)
9. [Encounter system](#9-encounter-system)
10. [World state](#10-world-state)
11. [Respawn flow](#11-respawn-flow)
12. [Projectile system](#12-projectile-system)
13. [Door and transition system](#13-door-and-transition-system)
14. [Checkpoint system](#14-checkpoint-system)
15. [Interaction system](#15-interaction-system)
16. [Dialogue system](#16-dialogue-system)
17. [Camera director](#17-camera-director)
18. [GameDirector](#18-gamedirector)
19. [Component list](#19-component-list)
20. [Runtime managers](#20-runtime-managers)
21. [Team ownership](#21-team-ownership)
22. [Development slices](#22-development-slices)
23. [Not building](#23-not-building)

---

## 1. Content pipeline

Three layers: a manifest mapping IDs to files, type files defining default object data, and scene instances placing objects with optional overrides.

### Object manifest

Maps stable string IDs to `.tgo` paths. Scene files and code reference IDs only. All entries are validated at startup — a missing file is a startup error.

```
ground_corner_ne     = Objects/Ground/ground_corner_ne.tgo
ground_corner_nw     = Objects/Ground/ground_corner_nw.tgo
ground_edge_n        = Objects/Ground/ground_edge_n.tgo
wall_long            = Objects/World/wall_long.tgo
wall_short           = Objects/World/wall_short.tgo
enemy_melee_basic    = Objects/Enemies/enemy_melee_basic.tgo
enemy_melee_fast     = Objects/Enemies/enemy_melee_fast.tgo
enemy_melee_tank     = Objects/Enemies/enemy_melee_tank.tgo
enemy_roll_basic     = Objects/Enemies/enemy_roll_basic.tgo
checkpoint_basic     = Objects/World/checkpoint_basic.tgo
door_level           = Objects/World/door_level.tgo
npc_hub              = Objects/World/npc_hub.tgo
pickup_health        = Objects/Pickups/pickup_health.tgo
```

### Type files (.tgo)

Each `.tgo` defines default values for a named object type. The `factoryType` field selects which factory function constructs it.

```
// Objects/Enemies/enemy_melee_basic.tgo
factoryType          = EnemyMelee
modelPath            = Models/Enemies/melee_basic.fbx
colliderType         = capsule
colliderRadius       = 0.4
colliderHeight       = 1.8
collisionLayer       = Enemy
maxHealth            = 3
damage               = 1
moveSpeed            = 3.2
aggroRange           = 7.0
attackRange          = 1.1
windupTime           = 0.35
recoveryTime         = 0.7
knockbackResistance  = 0.0
```

```
// Objects/Ground/ground_corner_ne.tgo
factoryType          = StaticWorld
modelPath            = Models/Ground/ground_corner_ne.fbx
colliderType         = box
colliderHalfExtents  = (100, 20, 100)
colliderOffset       = (0, -20, 0)
collisionLayer       = WorldStatic
```

### Scene instances

Scene objects reference a type by ID. The type file supplies all defaults; the scene instance stores only placement and overrides.

```
// Placed ground piece
name     = Ground_01
typeId   = ground_corner_ne
position = (0, 0, 0)
rotation = (0, 0, 0, 1)
scale    = (1, 1, 1)

// Enemy assigned to an encounter
name        = Enemy_MeleeA
typeId      = enemy_melee_basic
position    = (12, 0, 8)
encounterId = lvl1_room2

// Same type, stat override
name        = Enemy_MeleeB
typeId      = enemy_melee_basic
position    = (15, 0, 8)
encounterId = lvl1_room2
damage      = 2

// Checkpoint
name         = CP_01
typeId       = checkpoint_basic
position     = (30, 0, 0)
checkpointId = lvl1_cp_01

// Door — encounter lock + level transition
name          = DoorToHub
typeId        = door_level
position      = (60, 0, 0)
targetScene   = hub
targetSpawn   = hub_from_lvl1
encounterId   = lvl1_room2
```

### Runtime construction pipeline

1. `SceneImportService` parses the `.tgs` file into `SceneObjectData` structs.
2. `ObjectManifest::Resolve(typeId)` returns the `.tgo` path.
3. The `.tgo` is parsed into `TypeData` (`factoryType` enum + default `PropertyBag`).
4. Type defaults and scene overrides are merged into a `ResolvedObjectDef`. Scene values win on conflict.
5. `GameObjectFactory::Build(factoryType, resolvedDef)` constructs the `GameObject` and attaches components.
6. The factory function registers the object with relevant runtime systems.

```
struct SceneObjectData
    name        : string
    typeId      : string
    position    : Vector3
    rotation    : Quaternion
    scale       : Vector3
    properties  : PropertyBag

struct ResolvedObjectDef
    factoryType : FactoryType
    name        : string
    position    : Vector3
    rotation    : Quaternion
    scale       : Vector3
    properties  : PropertyBag   // archetype defaults + scene overrides merged
```

---

## 2. Factory types

`FactoryType` is an enum. The manifest loader maps the string in the `.tgo` to the enum at load time. All other usage is type-safe.

```
enum FactoryType
    StaticWorld
    Player
    EnemyMelee
    EnemyRoll
    EnemyRanged
    Checkpoint
    Door
    SpawnMarker
    EncounterTrigger
    PickupHealth
    NpcHub
    DeathVolume
    Decoration

class GameObjectFactory
    Register(type : FactoryType, fn : function)
    Build(def : ResolvedObjectDef) -> GameObject

    factories : map<FactoryType, function>
```

---

## 3. Object layers

Set by the factory function on construction. Not a scene-authored property. Used by the collision system and combat queries.

```
enum ObjectLayer
    WorldStatic
    Player
    Enemy
    Projectile
    Trigger
    Pickup
    NPC
```

---

## 4. Static world objects

All static geometry uses one factory function. Mesh path and collider parameters come from the resolved definition. Adding a new piece requires a `.tgo` file and a manifest entry — no code changes.

```
function BuildStaticWorld(def : ResolvedObjectDef) -> GameObject
    obj = new GameObject(def.name)
    obj.SetLayer(ObjectLayer.WorldStatic)
    obj.transform.position = def.position
    obj.transform.rotation = def.rotation

    obj.AddComponent(MeshComponent, def.properties["modelPath"])

    if def.properties["colliderType"] == "box"
        halfExtents = def.properties["colliderHalfExtents"]
        offset      = def.properties["colliderOffset"] or Vector3.Zero
        obj.AddComponent(BoxCollider, halfExtents, offset)
    else if def.properties["colliderType"] == "capsule"
        // ...

    return obj
```

---

## 5. Auto-tiling

Auto-tiling is a pipeline-time system that resolves which mesh variant to use for a static world tile based on its neighbors. Level designers paint intent (floor, wall, cliff) rather than placing specific corner or edge variants. The system selects the correct mesh automatically.

**Owner:** Pipeline/Combat. Auto-tiling hooks directly into the `StaticWorld` factory function — it runs during scene load before any `GameObject` is constructed.

### How it works

The scene importer does a first pass to populate a `TileGrid` — a flat 2D map from `(int x, int z)` tile coordinate to `TileType`. On the second pass, when the `StaticWorld` factory function constructs each auto-tiled object, it queries the grid for its 4 cardinal neighbors and computes a bitmask:

```
neighbor above  → +1
neighbor right  → +2
neighbor below  → +4
neighbor left   → +8
```

Each of the 16 possible bitmask values (0–15) maps to a specific mesh variant via a lookup table stored in the type's `.tgo` file. The factory substitutes the resolved `modelPath` before building the object. No extra scene data is needed beyond grid position and tile type.

```
// computed per tile during factory construction
bitmask = 0
if grid.Has(x,   z-1, tileType) then bitmask += 1
if grid.Has(x+1, z,   tileType) then bitmask += 2
if grid.Has(x,   z+1, tileType) then bitmask += 4
if grid.Has(x-1, z,   tileType) then bitmask += 8

meshPath = def.properties["autoTileMesh_" + bitmask]
```

Scene objects that use auto-tiling do not specify a `modelPath`. They specify a `tileType` and a `gridX` / `gridZ` coordinate instead.

```
// Scene instance — auto-tiled floor tile
name      = Floor_42
typeId    = floor_stone
tileType  = floor
gridX     = 5
gridZ     = 3
position  = (...)
```

Objects that don't participate in auto-tiling (enemies, doors, decorations) are unaffected by this system.

### Asset counts by object type

The number of mesh variants required depends on the topology of each object type. All counts assume the art pipeline allows mesh rotation — one mesh can be reused at 90°, 180°, and 270° rotations, which significantly reduces the unique asset count.

| Type | Combinations | Unique meshes (with rotation) | Notes |
|---|---|---|---|
| Ground / floor | 16 | 6 | Isolated, edge, corner, straight, T-junction, surrounded |
| Cliff edge | 16 | 6 | Same topology as ground but vertical face needed |
| Wall (linear) | 16 | 8 | Inner and outer corners are geometrically distinct — cannot be derived from each other by rotation |
| Decoration | 1 | 1 | No auto-tiling needed |

#### Ground tiles — 6 unique meshes

Ground tiles sit flat on the XZ plane. All 4 neighbors are on the same surface so the full 16-combination set applies. With rotation:

| Mesh | Combinations it covers | Description |
|---|---|---|
| `floor_isolated` | 0 | No neighbors — standalone tile |
| `floor_edge` | 1, 2, 4, 8 | Open on 3 sides — one neighbor |
| `floor_corner` | 3, 6, 9, 12 | Open on 2 adjacent sides |
| `floor_straight` | 5, 10 | Open on 2 opposite sides — corridor |
| `floor_t` | 7, 11, 13, 14 | Open on 1 side — T-junction |
| `floor_surrounded` | 15 | All 4 neighbors present — interior tile |

6 unique meshes cover all 16 combinations. Most of the scene will be `floor_surrounded` (interior) or `floor_edge` (room boundary), so art effort concentrates on those two.

#### Walls — 8 unique meshes

Walls are vertical and connect to adjacent wall tiles on the XZ plane. Inner and outer corners look fundamentally different — an inner corner is a concave join between two wall faces, an outer corner is a convex one — so rotation alone cannot derive one from the other. This adds 2 extra meshes compared to ground.

| Mesh | Description |
|---|---|
| `wall_isolated` | Standalone pillar / post |
| `wall_end` | Connected on one side only — end cap |
| `wall_straight` | Connected on two opposite sides |
| `wall_corner_outer` | Connected on two adjacent sides — convex corner |
| `wall_corner_inner` | Connected on two adjacent sides — concave corner |
| `wall_t` | Connected on three sides |
| `wall_cross` | Connected on all four sides |
| `wall_corner_outer_alt` | Optional second outer corner shape for visual variety |

The practical minimum is 7. The eighth slot is available for variation if the art team wants it.

#### Cliffs

Cliff edges follow the same 6-mesh pattern as ground tiles but the mesh has a visible vertical face on the open sides. Same asset count, slightly more complex geometry per piece.

### Important constraint: confirm tile size early

All auto-tiled objects must share the same grid unit size. This needs to be agreed on between programmers and artists before any tile assets are made. Once tile meshes exist at a given size, changing the grid unit breaks everything. Set it early and treat it as fixed.

---

## 6. Player stack

| Part | Responsibility |
|---|---|
| `PlayerController` | Reads raw input. Emits typed commands: move vector, melee press, melee hold, dodge press, ranged press, interact press. |
| `PlayerMotor` | Walking, facing direction, velocity, attack impulses, dodge movement, knockback, collision resolution. |
| `PlayerActionStateMachine` | State transitions, combo windows, cancel rules, lockout timing, i-frame windows, input buffering. |
| `PlayerResourceState` | HP (int), ammo (int), alive/dead flag, invuln timer, combo state, knockdown state. |

```
enum PlayerState
    Free
    Attack1, Attack2, Attack3
    Charge, ChargedAttack
    Dodge, DodgeAttack
    RangedAttack
    Interact
    Hurt, Knockdown
    Dead
```

---

## 7. Combat system

`CombatSystem` is the sole authority for damage resolution. Components do not apply damage to each other directly.

### Responsibilities

- Check active hitbox volumes against registered hurtboxes each frame
- Produce `HitEvent` structs for valid overlaps
- Track which objects have already been hit per swing to prevent duplicate hits
- Apply damage via `HealthComponent`
- Apply knockback and stun time via `PlayerMotor` / enemy controller
- Trigger hitstop on hit and call `CameraDirector::AddShake()`
- Grant ammo on melee hit where `AttackData::ammoGainOnHit > 0`

### Data structures

```
struct HitEvent
    attacker        : GameObject
    target          : GameObject
    damage          : int
    knockbackImpulse : Vector3
    stunTime        : float
    causesKnockdown : bool
    grantsAmmo      : bool
    attackType      : AttackType

struct AttackData
    id              : string
    damage          : int
    startupTime     : float
    activeTime      : float
    recoveryTime    : float
    movementImpulse : Vector3
    iFrameStart     : float
    iFrameEnd       : float
    ammoGainOnHit   : int
    causesKnockdown : bool
    hitstopDuration : float

enum AttackType
    MeleeLight, MeleeCombo, MeleeCharged
    DodgeAttack, Ranged
    EnemyMelee, EnemyRoll
```

All attacks — player combo hits, charged attack, dodge attack, ranged shot, all enemy attacks — are `AttackData` instances loaded at startup.

---

## 8. Enemies

One controller per behavior family. Stat variation across variants is expressed through config values, not separate controllers.

### MeleeEnemyController

```
enum MeleeState
    Idle, Chase, Windup, Attack, Cooldown, Hurt, Dead

struct MeleeEnemyConfig
    maxHealth           : int
    damage              : int
    moveSpeed           : float
    aggroRange          : float
    attackRange         : float
    windupTime          : float
    recoveryTime        : float
    knockbackResistance : float
```

**Navigation:** direct steering toward player position each frame. Collision sliding against `WorldStatic` handles obstacle avoidance.

| Type | Health | Speed | Windup |
|---|---|---|---|
| `enemy_melee_basic` | 3 | 3.2 | 0.35s |
| `enemy_melee_fast`  | 2 | 4.4 | 0.20s |
| `enemy_melee_tank`  | 5 | 2.1 | 0.55s |

### RollingEnemyController

```
enum RollState
    Idle, AcquireTarget, Aim, Roll, StunnedFromWall, Recover, Dead

struct RollingEnemyConfig
    maxHealth    : int
    rollSpeed    : float
    aimTime      : float
    stunDuration : float
    recoverTime  : float
```

**Navigation:** during `Aim`, samples player position and computes a fixed roll direction. During `Roll`, moves in that direction with no steering correction. Collision with `WorldStatic` triggers `StunnedFromWall`.

### RangedEnemyController (optional)

```
enum RangedState
    Idle, Approach, Shoot, Reposition, Cooldown, Hurt, Dead

struct RangedEnemyConfig
    maxHealth       : int
    moveSpeed       : float
    preferredRange  : float
    shootCooldown   : float
    projectileDamage : int
```

---

## 9. Encounter system

An encounter wakes enemies, locks doors, and clears when all member enemies are dead.

### Scene authoring

Three object types participate via the `encounterId` property:

| Object | Role |
|---|---|
| `EncounterTrigger` | Activates the encounter when the player enters. |
| Enemy instance | Any enemy with `encounterId` registers with that encounter. Starts `Idle` until activated. |
| Door instance | Any door with `encounterId` is controlled by that encounter. Closes on activation, opens on clear. |

### Lifecycle

1. **Registration** — factory functions for enemies and doors call `EncounterManager::RegisterEnemy(id, ptr)` / `RegisterDoor(id, ptr)` if `encounterId` is set.
2. **Trigger** — player enters `EncounterTrigger` → `EncounterManager::Activate(id)`.
3. **Wake** — manager calls `OnEncounterActivated()` on all registered enemies. Controllers transition to `Chase` / `AcquireTarget`.
4. **Lock** — manager calls `OnEncounterActivated()` on all registered doors. Doors close.
5. **Enemy death** — controller calls `EncounterManager::OnEnemyDied(id, ptr)`. Manager decrements alive count.
6. **Clear** — alive count reaches zero → `GameDirector::MarkEncounterCleared(id)` → `OnEncounterCleared()` on all doors.
7. **Unlock** — doors open.

If `GameDirector` already has the encounter ID in `myClearedEncounters` at scene load, the `EncounterTrigger` is inert and member enemies are not instantiated.

---

## 10. World state

Held in memory by `GameDirector`. Persists across respawns within a session.

```
clearedEncounters : set<string>
collectedPickups  : set<string>
```

Factory functions check both sets before instantiating. Enemies belonging to a cleared encounter are skipped. Pickups in the collected set are skipped.

---

## 11. Respawn flow

1. Player HP reaches 0 → `PlayerActionStateMachine` enters `Dead`.
2. Death animation completes → `GameDirector::Respawn()`.
3. Active scene reloads. All objects re-instantiate from scene data.
4. Factory functions check world state — cleared encounter enemies and collected pickups are skipped.
5. Player placed at `CheckpointManager::GetRespawnPosition()` with full HP and ammo.

---

## 12. Projectile system

```
struct ProjectileData
    startPosition : Vector3
    direction     : Vector3
    speed         : float
    maxRange      : float
    damage        : int
    ownerLayer    : ObjectLayer
    pierces       : bool

class ProjectileManager
    Spawn(data : ProjectileData)
    Update(dt : float)

    activeProjectiles : list<Projectile>
```

Used by player ranged attacks and the optional ranged enemy.

---

## 13. Door and transition system

A door can serve as an encounter lock, a level transition, or both simultaneously.

```
// DoorComponent — authored properties
encounterId      : string   // if set: controlled by EncounterManager
targetScene      : string   // if set: level transition door
targetSpawnId    : string
requiresInteract : bool
startsLocked     : bool
```

---

## 14. Checkpoint system

```
class CheckpointManager
    RegisterCheckpoint(id : string, position : Vector3)
    Activate(id : string)

    GetActiveId()       -> string
    GetRespawnPosition() -> Vector3
```

One active checkpoint at a time. Activates on player overlap via `CheckpointComponent`.

---

## 15. Interaction system

- Objects with `InteractionComponent` register with `InteractionManager` on init.
- Each frame: nearest registered interactable within player range is selected.
- UI prompt shown when a valid target exists.
- On interact input: `InteractionComponent::OnInteract()` called on the selected object.

---

## 16. Dialogue system

Stage-indexed strings on `DialogueComponent`. Current stage read from `GameDirector::GetStage()`. Stage advances when `GameDirector` marks a level completed.

```
// DialogueComponent — authored properties
stage0 : string
stage1 : string
stage2 : string
stage3 : string
```

---

## 17. Camera director

| Feature | Detail |
|---|---|
| Follow | Semi-locked follow with configurable isometric offset and positional damping. |
| Screen shake | Impulse-based, time-decaying. `CameraDirector::AddShake(magnitude, duration)`. Called by `CombatSystem` on hit. |
| Room bounds | Optional axis-aligned box clamping camera position for specific rooms. |

---

## 18. GameDirector

```
class GameDirector
    IsEncounterCleared(id : string)  -> bool
    MarkEncounterCleared(id : string)

    IsPickupCollected(id : string)   -> bool
    MarkPickupCollected(id : string)

    GetStage()    -> int
    AdvanceStage()

    LoadScene(sceneName : string, spawnId : string)
    Respawn()

    clearedEncounters    : set<string>
    collectedPickups     : set<string>
    stage                : int
    activeCheckpointId   : string
    currentScene         : string
```

---

## 19. Component list

### Engine-level

| Component | Notes |
|---|---|
| `MeshComponent` | Static mesh rendering. |
| `AnimatedMeshComponent` | Skeletal animation. Player and enemies. |
| `BoxColliderComponent` / `CapsuleColliderComponent` | Physics colliders. |
| `TriggerComponent` | Overlap detection. Encounter triggers, checkpoints, pickups. |
| `AudioEmitterComponent` | Plays audio clips by ID via engine audio API. |

### Gameplay

| Component | Owner | Notes |
|---|---|---|
| `HealthComponent` | Integration | Integer HP. I-frame timer. `OnDamage` / `OnDeath` callbacks. |
| `HurtboxComponent` | Pipeline/Combat | Registers object as a hittable target with `CombatSystem`. |
| `HitboxEmitterComponent` | Pipeline/Combat | Active hitbox volume for a timed duration. Used during attack active frames. |
| `ProjectileComponent` | Pipeline/Combat | Per-projectile data and movement. Managed by `ProjectileManager`. |
| `EnemyComponent` | Enemies | Stores `encounterId` and `ObjectLayer`. Shared base for all enemy controllers. |
| `PlayerComponent` | Player/Camera | Holds `PlayerResourceState`. Read by UI and other systems. |
| `CheckpointComponent` | World/Progression | Calls `CheckpointManager::Activate(id)` on player overlap. |
| `DoorComponent` | World/Progression | Encounter lock and/or level transition. |
| `InteractionComponent` | World/Progression | Registers as interactable. Exposes `OnInteract` callback. |
| `PickupComponent` | World/Progression | Calls `GameDirector::MarkPickupCollected`, applies effect, destroys self on overlap. |
| `DialogueComponent` | World/Progression | Stage-indexed dialogue strings. Read on interact. |

---

## 20. Runtime managers

| Manager | Owner | Responsibility |
|---|---|---|
| `ObjectManifest` | Pipeline/Combat | ID → `.tgo` path resolution. Startup validation. |
| `GameObjectFactory` | Pipeline/Combat | `FactoryType` → factory function dispatch. |
| `TileGrid` | Pipeline/Combat | 2D map of `(x, z)` → `TileType`. Populated on scene load. Queried by auto-tiling during `StaticWorld` construction. |
| `CombatSystem` | Pipeline/Combat | Hitbox resolution, `HitEvent` production, hitstop, feedback. |
| `ProjectileManager` | Pipeline/Combat | Projectile movement, collision, lifetime, despawn. |
| `CameraDirector` | Player/Camera | Follow, damping, screen shake, room bounds. |
| `EnemyManager` | Enemies | Tracks all active enemies. |
| `GameDirector` | World/Progression | World state, scene transitions, respawn, stage progression. |
| `EncounterManager` | World/Progression | Registration, activation, alive tracking, clear detection. |
| `CheckpointManager` | World/Progression | Active checkpoint and respawn position. |
| `InteractionManager` | World/Progression | Nearest interactable selection and prompt. |

---

## 21. Team ownership

### Pipeline + Combat
`ObjectManifest` · `GameObjectFactory` · `SceneObjectData` · `ResolvedObjectDef` · all factory functions · type `.tgo` authoring · `TileGrid` · auto-tiling system · tile mesh lookup tables · `CombatSystem` · `AttackData` · `HitEvent` · `HurtboxComponent` · `HitboxEmitterComponent` · `ProjectileManager` · `ProjectileComponent` · debug tooling

### Player + Camera
`PlayerController` · `PlayerMotor` · `PlayerActionStateMachine` · `PlayerResourceState` · `PlayerComponent` · `CameraDirector`

### Enemies
`EnemyManager` · `EnemyComponent` · `MeleeEnemyController` · `MeleeEnemyConfig` · `RollingEnemyController` · `RollingEnemyConfig` · `RangedEnemyController` (optional)

### World + Progression
`GameDirector` · `EncounterManager` · `CheckpointManager` · `CheckpointComponent` · `DoorComponent` · `DoorManager` · `InteractionManager` · `InteractionComponent` · `DialogueComponent` · `PickupComponent`

### Integration support
`HealthComponent` · `AudioEmitterComponent` · UI/HUD · cross-system integration

---

## 22. Development slices

Each slice ends with a playable build.

### Slice 1 — Skeleton
- Manifest, one factory function (`StaticWorld`), scene loads a room
- Player 8-direction movement, dodge, one melee attack, camera follow
- One melee enemy: chase, attack, death
- Hitbox/hurtbox overlap, damage, player knockback
- Player death: scene reload placeholder

### Slice 2 — Combat
- 3-hit combo, charged attack, dodge attack
- Ranged attack + ammo (gained on melee hit)
- Rolling enemy
- Screen shake, hitstop
- Health pickups

### Slice 3 — World loop
- Encounter system: trigger, room lock, clear, doors
- Checkpoints + respawn with persistent world state
- Door transitions: level ↔ hub
- Hub NPC with stage-based dialogue
- Level 1 full playthrough

### Slice 4 — Content
- Levels 2 and 3
- Optional ranged enemy
- Enemy variant types (fast, tank)
- Balancing, polish, cuts

---

## 23. Not building

- Behavior trees
- Navmesh / pathfinding framework
- Full quest system
- Generic ability framework
- Cross-session save/load
- Event-bus architecture
- Boss enemy *(possible cut)*
- Fall-height system *(possible cut)*