# Game Object System Guide

Last validated: 2026-04-10

## Audience and Goal

This guide is for programmers who add, inspect, or debug game-side objects.

It covers:

- How object data is converted into runtime objects.
- How lifetime works from creation through destruction.
- How `Essentials` is used for singleton access.
- Which singleton paths have not yet been migrated to `Essentials`.

## Runtime Overview

At runtime, object construction follows this path:

1. `GameWorld` calls `SceneImportService::BuildGameObjects(scenePath, manifest, archetypeLoader)`.
2. `SceneImportService` imports scene objects as `SceneObjectData`.
3. Each `typeId` resolves through `objects.manifest` to a `.tgo` archetype file.
4. Archetype defaults are merged with scene overrides into `ResolvedObjectDef`.
5. `GameObjectFactory` dispatches by `FactoryType` to a registered builder.
6. The builder creates a `GameObject`, adds components, and returns it to `GameWorld`.
7. `GameWorld` calls `Init` once, then runs `Update` and `Render` each frame.

Ownership is defined as follows:

- `GameWorld` owns `GameObject` instances through `std::unique_ptr`.
- Each `GameObject` owns its `Component` instances through `std::unique_ptr`.

## Runtime Lifetime Flow

### Startup

Entry and main loop are in `Source/Game/source/Go.cpp`.

1. `Tga::LoadSettings(...)`
2. `Tga::Engine::Start()`
3. Construct `GameWorld`
4. `GameWorld::Init()`
   - `RegisterGameObjectFactories()`
   - `myObjectManifest.Load("objects.manifest")`
   - `myObjectManifest.ValidateAll()`
   - Start the initial scene load with `StartSceneLoadAsync(...)`

### Scene Load and Object Construction

`GameWorld::StartSceneLoadAsync(...)` currently runs synchronously because the async path is compiled out.

Load path:

1. `GameWorld::LoadScene(...)` calls `SceneImportService::BuildGameObjects(scenePath, myObjectManifest, myArchetypeLoader)`.
2. Inside `SceneImportService::BuildGameObjects(...)`:
   - Import scene instances with `LoadSceneObjects(...)`.
   - Resolve each `typeId` with `ObjectManifest::Resolve(...)`.
   - Load the archetype with `ArchetypeLoader::Load(...)`.
   - Merge properties so scene values override archetype defaults.
   - Convert the archetype factory string with `FactoryTypeFromString(...)`.
   - Build the object through `GameObjectFactory::GetInstance().Build(resolvedDef)`.
3. `GameWorld::ApplyLoadedScene(...)`:
   - Clears non-persistent objects.
   - Calls `object->Init(*engine)`.
   - Moves each object into `myGameObjects`.

### Per-Frame Runtime

`Go.cpp` loop:

1. `gameWorld.Update(engine.GetDeltaTime())`
2. `gameWorld.Render()`
3. `engine.EndFrame()`

`GameWorld::Update(...)` then:

1. Updates timer, input, and scene switching logic.
2. Updates camera systems.
3. Updates each active game object.
4. Updates VFX and audio.

`GameWorld::Render(...)` draws the loading screen or delegates to `SceneRenderer`.

### Teardown

When the loop exits:

1. `GameWorld` goes out of scope.
2. `myGameObjects` is cleared.
3. `GameObject` destructor calls `RemoveAllComponents()`.
4. `RemoveAllComponents()` calls `OnDestroy()` in reverse component order.
5. Engine shutdown occurs through `Tga::Engine::GetInstance()->Shutdown()`.

## Ownership and Lifetime Rules

### `GameWorld` -> `GameObject`

- The world stores objects in `std::vector<std::unique_ptr<GameObject>>`.
- Clearing or destroying that container destroys the owned objects automatically.

### `GameObject` -> `Component`

- Components are stored in `std::vector<std::unique_ptr<Component>>`.
- `AddComponent<T>(...)` sets the owner immediately.
- If the object is already initialized, the new component's `Init(...)` runs immediately.
- If the object is not initialized yet, component `Init(...)` is deferred until object `Init(...)`.

### `Component` vs `ScriptComponent`

`Component` is the base type for all attachable behaviors and exposes the full lifecycle surface:

- `Init(...)`
- `FixedUpdate(...)`
- `Update(...)`
- `LateUpdate(...)`
- `Render()`
- `OnDestroy()`
- `OnActiveChanged(...)`
- `OnEnabledChanged(...)`

`ScriptComponent` is a convenience layer built on top of `Component` for script-style gameplay behavior.

Current source behavior:

- `ScriptComponent::FixedUpdate(...)`, `Update(...)`, and `LateUpdate(...)` are `final`.
- `OnStart()` runs once per component lifetime before the first script phase callback.
- `OnFixedUpdate(...)` runs in the fixed-step pass.
- `OnUpdate(...)` runs in the frame update pass.
- `OnLateUpdate(...)` runs after frame update.
- `OnEnable()`/`OnDisable()` run when runtime-active state changes (`GameObject::IsActive()` combined with `Component::IsEnabled()`).
- `OnScriptDestroy()` runs during component teardown.

Usage guideline:

- Use `Component` when manual lifecycle control is required, such as rendering, collider state, or explicit init/update wiring.
- Use `ScriptComponent` for gameplay logic that benefits from script-style hooks (`OnStart`, `OnEnable`, `OnFixedUpdate`, `OnUpdate`, `OnLateUpdate`, `OnDisable`, `OnScriptDestroy`).

### Active vs Enabled

- `GameObject::SetActive(...)` toggles object-wide activity.
- When active state changes, all components receive `OnActiveChanged(...)`.
- Per-component enable state is controlled through `Component::SetEnabled(...)` and triggers `OnEnabledChanged(...)` on state transitions.
- `Update` and `Render` run only when the object is active and the component is enabled.

Runtime phase order for objects:

- `FixedUpdate` (60 Hz fixed timestep)
- `Update` (frame timestep)
- `LateUpdate` (frame timestep)

### Scene Persistence

`GameWorld::ClearSceneObjects()` removes only non-persistent objects.

- Objects with `IsPersistent() == true` remain across scene transitions.
- Persistent objects should be used carefully to avoid duplicate instances or stale state.

### Destruction Order

- `RemoveAllComponents()` iterates in reverse order.
- This supports LIFO-style cleanup for component dependencies.

## Data and Property Pipeline

### Scene Import

`SceneImportService` extracts runtime-oriented data from `.tgs` content:

- Name, transform, and type id.
- Selected properties including float, int, bool, string, vector, color, model, and render mode.

`SceneObjectData` is the transport struct for imported data and stores property values in `std::any`.

### Manifest Resolution

`objects.manifest` maps each `typeId` to an archetype path.

Example entries:

```txt
test_static = Objects/test_static.tgo
test_enemy = Objects/test_enemy.tgo
```

`ObjectManifest::ValidateAll()` runs at startup and throws if entries are missing.

### Archetype Parsing

`ArchetypeLoader::Load(...)` supports:

- JSON `.tgo` format.
- Legacy `key=value` format.

Required field:

- `factoryType` must exist. Load fails if it is missing.

### Merge Rule

In `SceneImportService::BuildGameObjects(...)`, merge order is:

- Start with archetype defaults.
- Overwrite matching keys with scene instance properties.

When keys collide, scene values take precedence.

## Factory System

Key files:

- `FactoryType.h`
- `GameObjectFactory.h/.cpp`
- `GameObjectFactoryRegistrations.cpp`

Current registration function `RegisterGameObjectFactories()` registers builders for:

- `FactoryType::StaticWorld`
- `FactoryType::EnemyMelee`

Factory dispatch is strict. If a type is not registered, `Build(...)` logs an error and returns null.

## How To Add a New Object Type

Checklist for adding a new gameplay object category:

1. Add a new enum value in `FactoryType.h`.
2. Add string-to-enum mapping in `FactoryTypeFromString(...)` in `GameObjectFactory.cpp`.
3. Implement the builder function in `GameObjectFactoryRegistrations.cpp`.
4. Register the builder in `RegisterGameObjectFactories()`.
5. Add or extend the archetype `.tgo` with `factoryType` and default values.
6. Add a `typeId = path/to_file.tgo` entry in `objects.manifest`.
7. Place the object in a scene with the matching `typeId`.
8. Run and verify that:
   - the object instantiates,
   - components initialize,
   - the object updates and renders,
   - scene transitions preserve expected lifetime behavior.

## Practical Walkthrough: Player Object Lifecycle

This section traces a single Player object through the runtime pipeline. The snippets are pseudocode and show sequencing and system boundaries.

### 1) Authoring: `player.tgo` and manifest mapping

Authoring starts with data. The archetype defines default values for Player instances unless a scene overrides them.

```txt
# Objects/player.tgo (pseudocode fields)
factoryType     = Player
modelPath       = Models/Characters/player.fbx
colliderType    = capsule
colliderRadius  = 0.45
colliderHeight  = 1.80
colliderOffset  = (0, 0.90, 0)
maxHealth       = 6
moveSpeed       = 4.20
dodgeSpeed      = 9.00
```

Field usage:

- `factoryType` is the dispatch key converted to `FactoryType::Player` in `FactoryTypeFromString(...)` in `Source/Game/source/GameObjectFactory.cpp`.
- `modelPath` is used by rendering setup through the shared helper pattern `ApplyCommonTransformAndModel(...)` in `Source/Game/source/GameObjectFactoryRegistrations.cpp`.
- `collider*` values define the interaction shape for collision and combat overlap.
- `maxHealth` seeds gameplay state without hardcoding combat values in C++ at spawn time.

The manifest entry binds a stable scene-facing id to the archetype path:

```txt
# objects.manifest
player = Objects/player.tgo
```

This allows scenes and code to reference `typeId` values such as `player` instead of direct file paths. `ObjectManifest::ValidateAll()` in `Source/Game/source/GameWorld.cpp` catches missing archetype paths at startup.

### 2) Construction: what `BuildPlayer(...)` composes

When `SceneImportService::BuildGameObjects(...)` in `Source/Game/source/SceneImportService.cpp` resolves the Player archetype and merges scene overrides, it passes a `ResolvedObjectDef` to the factory.

The Player builder composes the runtime components required for the object.

```cpp
// Pseudocode only
std::unique_ptr<GameObject> BuildPlayer(const ResolvedObjectDef& def)
{
    auto obj = std::make_unique<GameObject>(def.name);
    obj->SetLayer(ObjectLayer::Player);

    ApplyCommonTransformAndModel(*obj, def);   // position/rotation/scale + model binding

    obj->AddComponent<AnimatedMeshComponent>(def["modelPath"]);
    obj->AddComponent<CapsuleColliderComponent>(
        def["colliderRadius"],
        def["colliderHeight"],
        def["colliderOffset"]);

    auto* health = obj->AddComponent<HealthComponent>(def["maxHealth"]);
    obj->AddComponent<HurtboxComponent>(ObjectLayer::Player);

    auto* playerData = obj->AddComponent<PlayerComponent>();
    playerData->SetResourceState(PlayerResourceState{/*hp*/def["maxHealth"], /*ammo*/0});

    auto* motor = obj->AddComponent<PlayerMotor>(def["moveSpeed"], def["dodgeSpeed"]);
    auto* actions = obj->AddComponent<PlayerActionStateMachine>(playerData, motor, health);
    obj->AddComponent<PlayerController>(actions);

    return obj;
}
```

Component roles:

- `AnimatedMeshComponent` handles visual representation.
- `CapsuleColliderComponent` provides a movement-oriented collision volume for character motion.
- `HealthComponent` stores survivability state such as HP and damage handling.
- `HurtboxComponent` exposes the combat-facing surface used by combat systems.
- `PlayerComponent` stores shared player state, including `PlayerResourceState`, for consumers such as UI and other runtime systems.
- `PlayerController`, `PlayerActionStateMachine`, and `PlayerMotor` separate input handling, rule evaluation, and movement execution.

This separation keeps input mapping, gameplay rules, and physical movement in distinct units.

### 3) Initialization: what happens after factory return

After construction, ownership moves to `GameWorld`. In `Source/Game/source/GameWorld.cpp`, `GameWorld::LoadScene(...)` imports and builds objects, then `GameWorld::ApplyLoadedScene(...)` calls `Init` before the objects are inserted into the active world list.

```cpp
// Pseudocode mirroring GameWorld flow
auto imported = SceneImportService::BuildGameObjects(scenePath, manifest, archetypeLoader);
ApplyLoadedScene(std::move(imported), scenePath);

for (auto& object : someObjects)
{
    object->Init(*engine);
    myGameObjects.push_back(std::move(object));
}
```

`GameObject::Init(...)` in `Source/Game/source/GameObject.cpp` then forwards initialization to each attached component in add order.

This phase is used for runtime binding such as engine handles, collision registration, animation runtime state, and callback wiring. Constructors assemble structure; `Init` performs runtime-side setup.

After initialization, the Player is active in the world and ready for frame updates.

### 4) Per-frame lifetime: how the Player runs each frame

Frame flow starts in `Source/Game/source/Go.cpp` and cascades through the object hierarchy:

```cpp
// Pseudocode mirroring Go.cpp and GameWorld.cpp
while (engine.BeginFrame())
{
    gameWorld.Update(dt);   // updates active objects
    gameWorld.Render();     // renders active objects
    engine.EndFrame();
}
```

Within that update chain (`GameWorld::Update(...)` -> `GameObject::Update(...)`), the Player stack can be viewed as:

```cpp
// Conceptual order inside Player components
commands = PlayerController::ReadInput();
decision = PlayerActionStateMachine::Evaluate(commands, resourceState, dt);
PlayerMotor::Apply(decision.motionIntent, dt);
PlayerComponent::Publish(resourceState);
```

- `PlayerController` converts device input into gameplay commands.
- `PlayerActionStateMachine` applies legal transition rules such as combo windows, cancel windows, lockouts, and invulnerability frames.
- `PlayerMotor` executes movement and impulses against collision.
- `PlayerComponent` publishes resulting `PlayerResourceState` for systems such as HUD and progression.

This layout allows input handling, combat rules, and movement behavior to be changed independently.

Render follows a parallel cascade (`GameWorld::Render(...)` -> scene renderer -> `GameObject::Render(...)` -> component `Render()`), where animation and mesh components consume already-resolved gameplay state.

### 5) Teardown: scene transition, respawn, and shutdown

The Player has three common teardown contexts.

On scene transition, `GameWorld::ClearSceneObjects()` in `Source/Game/source/GameWorld.cpp` removes non-persistent objects and keeps persistent ones. Persistence should be enabled only when cross-scene continuity is required.

On respawn, the flow described in `Doc/P4_System_Design.md` is: death state -> `GameDirector::Respawn()` -> scene reload -> world-state filtering -> player spawn at checkpoint with reset combat resources. This rebuilds player state from canonical systems instead of partially mutated runtime state.

On engine shutdown, scope exit in `Source/Game/source/Go.cpp` destroys `GameWorld`, which releases owned `GameObject` instances. `GameObject::~GameObject()` calls `RemoveAllComponents()` in `Source/Game/source/GameObject.cpp`, and `OnDestroy()` runs in reverse component order.

```cpp
// Pseudocode mirroring GameObject teardown
for (auto it = components.rbegin(); it != components.rend(); ++it)
{
    (*it)->OnDestroy();
}
```

Reverse-order teardown gives high-level behavior components an opportunity to unsubscribe and release references before lower-level components are destroyed.

## Essentials and Singleton Access

This section summarizes current usage and migration status.

### What `Essentials` Is

`Essentials` in `Source/Game/source/Essentials/Essentials.h` is a facade-style singleton used to centralize access to core global systems.

It exposes static members such as:

- `globalEngine`
- `globalInputManager`
- `globalCamera`
- `globalAudioManager`
- `ShutdownQueued`

It also exposes helper wrappers such as `GetDeltaTime`, `GetTotalTime`, resolution helpers, and `Shutdown`.

### Initialization and Shutdown Constraints

The `Essentials` constructor initializes handles using `Tga::Engine::GetInstance()`, so engine startup order matters.

Safe order:

1. `Tga::Engine::Start()` succeeds.
2. `Essentials::GetEssentials()` is called at least once.
3. `Essentials` static handles are used.

Current shutdown flow:

1. `WM_DESTROY` in `Go.cpp` calls `Essentials::Shutdown()`.
2. The main loop checks `Essentials::ShutdownQueued` and exits.
3. Normal teardown continues.

### How To Use It in Code

Bootstrap explicitly when a module depends on facade members:

```cpp
// After Engine::Start succeeds
Essentials::GetEssentials();
```

Then access facade members or helper functions:

```cpp
const float dt = Essentials::GetDeltaTime();
if (Essentials::ShutdownQueued)
{
    // graceful loop exit path
}
```

### Migration Status (Current)

Not all singleton access has been routed through `Essentials`.

| System | Access Pattern Today | Migration Status | Notes |
|---|---|---|---|
| `Tga::InputManager` | `Essentials::GetEssentials().globalInputManager` | Facade path in use | Used in `Go.cpp` window proc flow. |
| `AudioManager` | `Essentials::globalAudioManager...` and `AudioManager::GetInstance()` API exists | Mixed | Current codebase contains both patterns. Avoid introducing additional mixed styles in the same change. |
| `Tga::Engine` | Frequent direct `Tga::Engine::GetInstance()` plus `Essentials::globalEngine` | Not fully migrated | Direct engine singleton usage is still common across gameplay and rendering code. |
| `Tga::ModelFactory` | Direct `Tga::ModelFactory::GetInstance()` | Not migrated | Used by mesh and import paths. |
| `SceneManager` | Direct `SceneManager::GetInstance()` | Not migrated | Scene request and current-scene state are managed directly. |
| `GameObjectFactory` | Direct `GameObjectFactory::GetInstance()` | Not migrated | Factory registry and build dispatch remain direct singleton usage. |

### Practical Team Rule

For the current codebase:

1. If a system is already routed through `Essentials` in a module, continue using that path.
2. If a system is not migrated yet, use the existing direct singleton style used by that module.
3. Do not introduce a third access pattern for the same system in one module.
4. If a migration is made, update all local call sites in one coherent pass and document the change.

## End-to-End Example A: `StaticWorld`

Input assets:

- `objects.manifest`: `test_static = Objects/test_static.tgo`
- `Objects/test_static.tgo`: includes `factoryType = StaticWorld` and model data.

Runtime result:

1. A scene instance with `typeId = test_static` is imported.
2. The manifest resolves the archetype path.
3. The archetype loader sets `factoryTypeString = StaticWorld`.
4. Factory dispatch calls `BuildStaticWorld(...)`.
5. The object receives transform, layer, and model component setup.
6. An optional box collider is added if collider properties are present.

## End-to-End Example B: `EnemyMelee`

Input assets:

- `objects.manifest`: `test_enemy = Objects/test_enemy.tgo`
- `Objects/test_enemy.tgo`: includes `factoryType = EnemyMelee`, `health`, and `damage`.

Runtime result:

1. The scene instance is imported as `SceneObjectData`.
2. Merge logic applies scene overrides over archetype defaults.
3. Factory dispatch calls `BuildEnemyMelee(...)`.
4. The object receives enemy layer and model component setup.
5. `DamageableComponent` is attached and initialized with health and damage values.

## Troubleshooting

| Symptom | Likely Cause | Check |
|---|---|---|
| Object never appears | Unknown `typeId` or missing manifest entry | `objects.manifest`, `ObjectManifest::Resolve` logs |
| Scene load throws on startup | Missing or invalid archetype path | `ObjectManifest::ValidateAll`, verify archetype file exists |
| Build returns null | Factory type not registered | `RegisterGameObjectFactories`, `GameObjectFactory::Build` |
| Wrong stats or model in scene | Property merge or typing mismatch | Scene overrides, `std::any` type in `ResolvedObjectDef` |
| Component not initialized | Added before owner init and object init never ran | `GameObject::Init` call path |
| Object survives scene unexpectedly | Persistent flag left enabled | `SetPersistent`, `ClearSceneObjects` behavior |
| Singleton access seems inconsistent | Module mixes facade and direct singleton calls | `Essentials` section and local style in touched module |

## Quick Checklist for PR Review

Use this before opening a PR that touches object creation or lifetime:

1. Factory type enum, mapping, and registration are in sync.
2. Manifest entry exists and resolves to a real archetype.
3. Archetype has `factoryType` and valid property types.
4. The new builder sets the correct layer and required components.
5. Init, update, render, and teardown behavior were tested.
6. Scene transition behavior was tested, including persistence expectations.
7. Singleton access style is consistent within the module, or migration scope is explicit.

## Primary Reference Files

- `Source/Game/source/Go.cpp`
- `Source/Game/source/GameWorld.h`
- `Source/Game/source/GameWorld.cpp`
- `Source/Game/source/GameObject.h`
- `Source/Game/source/GameObject.cpp`
- `Source/Game/source/Component.h`
- `Source/Game/source/ObjectManifest.cpp`
- `Source/Game/source/ArchetypeLoader.cpp`
- `Source/Game/source/SceneImportService.cpp`
- `Source/Game/source/SceneImportService.h`
- `Source/Game/source/GameObjectFactory.cpp`
- `Source/Game/source/GameObjectFactoryRegistrations.cpp`
- `Source/Game/source/Essentials/Essentials.h`
- `Source/Game/source/Essentials/Essentials.cpp`
- `Source/Game/source/AudioManager.h`
- `Source/Game/source/AudioManager.cpp`
