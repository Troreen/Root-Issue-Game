# Object Pipeline Refactor Guide

## Glossary

### Concepts

| Term | What it is | Example | Analogy |
|---|---|---|---|
| **typeId** | A name on a scene instance that says "I'm a copy of this prefab." It's a string the scene editor stamps on every placed object. The pipeline uses it to find the `.tgo` file that holds the defaults. It is **not** the object's type in the C++ sense — multiple different typeIds can share the same builder. | `"enemy_melee_basic"`, `"enemy_melee_fast"` | A SKU number. "This particular box off the shelf." |
| **factoryType** | A string inside the `.tgo` file that selects which builder function constructs the object. This determines what components the object gets. Multiple typeIds can point to the same factoryType — they share the same builder but have different default values. | `"EnemyMelee"` (used by both `enemy_melee_basic.tgo` and `enemy_melee_fast.tgo`) | A product category. "All these SKUs are built on the same assembly line." |
| **Prefab (`.tgo` file)** | A data file containing default property values for a type of object. One file per typeId. Contains the `factoryType` string and any number of key-value properties (model path, health, speed, etc.). The name `.tgo` likely stands for "TGA Game Object." | `Objects/enemy_melee_basic.tgo` | A blueprint with default specs. |
| **Scene instance** | A placed object in a `.tgs` scene file. Has a name, typeId, transform (position/rotation/scale), and optional property overrides. At load time, its typeId resolves to a prefab, and the scene overrides are merged on top of the prefab defaults. | An enemy placed at position `(12, 0, 8)` with `typeId = enemy_melee_basic` and `damage = 2` (overriding the prefab default of `1`) | A physical item on the shelf — same blueprint as the others, but this one has a custom sticker. |
| **Builder function** | A C++ function that receives merged data (prefab defaults + scene overrides) and constructs a `GameObject` with the right components. One builder per factoryType. Registered at startup with a string key. | `BuildEnemyMelee(const SceneObjectData& aData)` | The assembly line for a product category. |
| **ObjectLayer** | An enum set by the builder that tags what kind of object this is for collision and combat queries. Not data-driven — hardcoded in the builder because it's fundamental to the object's role. | `ObjectLayer::Enemy`, `ObjectLayer::Player`, `ObjectLayer::WorldStatic` | A department label. "This goes in aisle 3." |

### How typeId and factoryType relate

This is the key relationship that can be confusing:

```
typeId = "enemy_melee_basic"  ──→  enemy_melee_basic.tgo  ──→  factoryType = "EnemyMelee"
typeId = "enemy_melee_fast"   ──→  enemy_melee_fast.tgo   ──→  factoryType = "EnemyMelee"
typeId = "enemy_melee_tank"   ──→  enemy_melee_tank.tgo   ──→  factoryType = "EnemyMelee"
```

Three different typeIds, three different `.tgo` files with different stat values, but all using the same builder. The builder gives them the same components (mesh, collider, health, AI controller). The `.tgo` files give them different numbers (health 2 vs 3 vs 5, speed 4.4 vs 3.2 vs 2.1).

```
typeId = "wall_long"          ──→  wall_long.tgo          ──→  factoryType = "StaticWorld"
typeId = "ground_corner_ne"   ──→  ground_corner_ne.tgo   ──→  factoryType = "StaticWorld"
```

Different typeIds, same builder. One's a wall, one's a floor piece, but both are static meshes with box colliders.

### Data types (after refactor)

| Type | Defined in | What it holds | Role |
|---|---|---|---|
| **`SceneObjectData`** | `SceneObjectData.h` | `name`, `typeId`, `position`, `rotation`, `scale`, `properties` (map of `string` → `std::any`) | The single data type flowing through the entire pipeline. Scene data comes in as this. Prefab defaults get merged into it. Builders receive it. |
| **`PrefabData`** | `SceneImportService.cpp` (local) | `factoryType` (string), `properties` (map of `string` → `std::any`) | Temporary struct returned by `ParsePrefab()`. Only exists long enough to merge into `SceneObjectData` and extract the `factoryType` string for dispatch. Not passed to builders. |
| **`GameObject`** | `GameObject.h` | Transform, name, tag, active/persistent flags, list of owned `Component` instances | The runtime object. What the builder produces. Owned by `GameWorld`. |
| **`Component`** | `Component.h` | Owner pointer, enabled flag, lifecycle hooks (`Init`, `Update`, `Render`, `OnDestroy`) | Base class for all behaviors attached to a `GameObject`. Builders compose objects by adding the right components. |

### Data types being removed

| Type | Was defined in | What it held | Why it's removed |
|---|---|---|---|
| **`ArchetypeData`** | `ArchetypeLoader.h` | `factoryTypeString` + property map | Same shape as `PrefabData`. The loader class is replaced by a free function. |
| **`ResolvedObjectDef`** | `ResolvedObjectDef.h` | `FactoryType` enum + name + transform + merged property map | Same shape as `SceneObjectData`. Existed only because it held the enum, which is now a string passed separately. |
| **`FactoryType` enum** | `FactoryType.h` | 13 enum values mapping to builder functions | Replaced by string-keyed dispatch. The string already exists in the `.tgo` file. |
| **`ObjectManifest`** | `ObjectManifest.h` | Map of typeId → `.tgo` file path | Replaced by path convention: typeId `"foo"` → `Objects/foo.tgo`. |

### Pipeline flow (after refactor)

```
Scene instance                     "What's placed here?"
    │
    │  typeId = "enemy_melee_fast"
    │  position, rotation, scale
    │  damage = 2  (override)
    │
    ▼
SceneObjectData                    "Raw scene data"
    │
    │  Convention: "Objects/" + typeId + ".tgo"
    │
    ▼
ParsePrefab(path)                  "What are the defaults?"
    │
    │  factoryType = "EnemyMelee"
    │  moveSpeed = 4.4, maxHealth = 2, damage = 1, ...
    │
    ▼
Merge                              "Combine defaults + overrides"
    │
    │  Scene damage = 2 wins over prefab damage = 1
    │  Prefab moveSpeed = 4.4 fills in (not in scene)
    │
    ▼
SceneObjectData (merged)           "Complete property set"
    │
    │  factory.Build("EnemyMelee", mergedData)
    │
    ▼
BuildEnemyMelee(data)             "Assemble the object"
    │
    │  Creates GameObject
    │  Adds MeshComponent, ColliderComponent,
    │  HealthComponent, EnemyAI, etc.
    │  Does NOT set transform — that's the caller's job
    │
    ▼
Apply transform                    "Position it in the world"
    │
    │  BuildSceneObjects sets position, rotation, scale
    │  from merged SceneObjectData — same for every object
    │
    ▼
GameObject                         "Live in the world"
```

---

## Why this refactor exists

The current pipeline has five representations of "what kind of object this is":

| Concept | Example | Where |
|---|---|---|
| `typeId` | `"enemy_melee_basic"` | Scene file |
| Manifest entry | `enemy_melee_basic = Objects/Enemies/enemy_melee_basic.tgo` | `objects.manifest` |
| `.tgo` file path | `Objects/Enemies/enemy_melee_basic.tgo` | Filesystem |
| `factoryType` string | `"EnemyMelee"` | Inside the `.tgo` |
| `FactoryType` enum | `FactoryType::EnemyMelee` | C++ code (`FactoryType.h`) |

There are also two intermediate data types that exist solely to shuttle data between pipeline stages:

| Type | What it holds | Defined in |
|---|---|---|
| `ArchetypeData` | `factoryTypeString` + property map | `ArchetypeLoader.h` |
| `ResolvedObjectDef` | `FactoryType` enum + name + transform + merged property map | `ResolvedObjectDef.h` |

Both are nearly identical to `SceneObjectData` (name + typeId + transform + property map, defined in `SceneObjectData.h`). The three types have the same `std::unordered_map<std::string, std::any> properties` and the same `TryGetProperty`/`GetPropertyOr`/`HasProperty` helpers duplicated across them.

The pipeline currently requires 7 steps to add a new object type (see `Object_System_Guide_neutral.md`, section "How To Add a New Object Type"). After this refactor, it requires 3.

---

## What changes

| Layer | Current | After refactor |
|---|---|---|
| **Type ID to file path** | `ObjectManifest` class + `objects.manifest` file | Convention: `typeId` maps to `Objects/{typeId}.tgo` |
| **Prefab file parsing** | `ArchetypeLoader` class + `ArchetypeData` struct | Free function `ParsePrefab()` returning directly into `SceneObjectData` |
| **Property merge** | Separate step in `GameWorld::BuildSceneObjects` producing `ResolvedObjectDef` | Merge happens in `BuildSceneObjects` directly into `SceneObjectData` |
| **Factory dispatch** | `FactoryType` enum + `FactoryTypeFromString()` + `Register(FactoryType, fn)` | `Register(string, fn)` — string-keyed dispatch |
| **Builder signature** | `Build(const ResolvedObjectDef&)` | `Build(const SceneObjectData&)` |

### Files removed

| File | Why |
|---|---|
| `FactoryType.h` | Enum replaced by string dispatch |
| `ResolvedObjectDef.h` | Redundant with `SceneObjectData` |
| `ArchetypeLoader.h` / `ArchetypeLoader.cpp` | Replaced by free function in `SceneImportService.cpp` |
| `ObjectManifest.h` / `ObjectManifest.cpp` | Replaced by path convention |
| `objects.manifest` (data file) | No longer needed |

### Files modified

| File | Changes |
|---|---|
| `GameObjectFactory.h` / `.cpp` | Registry key changes from `FactoryType` to `std::string`. `FactoryTypeFromString` removed. |
| `GameObjectFactoryRegistrations.cpp` | Builder signatures change from `ResolvedObjectDef` to `SceneObjectData`. Registration uses strings. |
| `GameWorld.h` / `.cpp` | `BuildSceneObjects` simplified. `ObjectManifest` and `ArchetypeLoader` members removed. |
| `SceneObjectData.h` | No structural changes. Already has everything builders need. |
| `SceneImportService.cpp` / `.h` | Gains the prefab parsing function (moved from `ArchetypeLoader`). |

---

## Step-by-step refactor

### Step 1: Change factory dispatch from enum to string

This is the lowest-risk change and can be done first, independently.

**Current state** (`GameObjectFactory.h`):

```cpp
#include "FactoryType.h"
#include "ResolvedObjectDef.h"

class GameObjectFactory
{
public:
    using FactoryFunction = std::function<std::unique_ptr<GameObject>(const ResolvedObjectDef&)>;

    void Register(FactoryType aType, FactoryFunction aFactory);
    std::unique_ptr<GameObject> Build(const ResolvedObjectDef& aDefinition) const;
    static GameObjectFactory& GetInstance();

private:
    std::unordered_map<FactoryType, FactoryFunction> myFactories;
};
```

**After**:

```cpp
// No more FactoryType.h include
#include "ResolvedObjectDef.h"  // keep for now, removed in step 3

class GameObjectFactory
{
public:
    // Builders still take ResolvedObjectDef for now — changed in step 3
    using FactoryFunction = std::function<std::unique_ptr<GameObject>(const ResolvedObjectDef&)>;

    void Register(const std::string& aType, FactoryFunction aFactory);
    std::unique_ptr<GameObject> Build(const std::string& aFactoryType,
                                      const ResolvedObjectDef& aDefinition) const;
    static GameObjectFactory& GetInstance();

private:
    std::unordered_map<std::string, FactoryFunction> myFactories;
};
```

**Changes in `GameObjectFactory.cpp`**:

Remove `FactoryTypeFromString()` entirely (the function at the bottom of the current file that maps lowercase strings to enum values). Remove the `TrimWhitespace` and `ToLower` helpers if they were only used by that function.

`Build` changes from:
```cpp
std::unique_ptr<GameObject> GameObjectFactory::Build(const ResolvedObjectDef& aDefinition) const
{
    auto it = myFactories.find(aDefinition.factoryType);
    if (it == myFactories.end())
    {
        ERROR_PRINT("No factory registered for type");
        return nullptr;
    }
    return it->second(aDefinition);
}
```

To:
```cpp
std::unique_ptr<GameObject> GameObjectFactory::Build(const std::string& aFactoryType,
                                                      const ResolvedObjectDef& aDefinition) const
{
    auto it = myFactories.find(aFactoryType);
    if (it == myFactories.end())
    {
        ERROR_PRINT("No factory registered for type: %s", aFactoryType.c_str());
        return nullptr;
    }
    return it->second(aDefinition);
}
```

**Changes in `GameObjectFactoryRegistrations.cpp`**:

```cpp
// Before
void RegisterGameObjectFactories()
{
    GameObjectFactory& factory = GameObjectFactory::GetInstance();
    factory.Register(FactoryType::StaticWorld, BuildStaticWorld);
}

// After
void RegisterGameObjectFactories()
{
    GameObjectFactory& factory = GameObjectFactory::GetInstance();
    factory.Register("StaticWorld", BuildStaticWorld);
}
```

**Changes in `GameWorld::BuildSceneObjects`** (`GameWorld.cpp`):

The line that converts the string to an enum:
```cpp
resolvedDefinition.factoryType = FactoryTypeFromString(archetypeData.factoryTypeString);
```

Is no longer needed. Instead, pass the string directly to `Build`:
```cpp
auto gameObject = GameObjectFactory::GetInstance().Build(
    archetypeData.factoryTypeString, resolvedDefinition);
```

**After this step**: `FactoryType.h` can be deleted. `ResolvedObjectDef.h` no longer needs to include it — change its `FactoryType factoryType` member to `std::string factoryType` (or remove it entirely since the factory type is now passed separately to `Build`).

---

### Step 2: Kill the manifest — use path convention

**Current state** (`GameWorld.h` members):

```cpp
ObjectManifest myObjectManifest;
ArchetypeLoader myArchetypeLoader;
```

**Current state** (`GameWorld::Init`):

```cpp
myObjectManifest.Load("objects.manifest");
myObjectManifest.ValidateAll();
```

**Current state** (`GameWorld::BuildSceneObjects`, the resolve step):

```cpp
const std::string archetypePath = myObjectManifest.Resolve(sceneObject.typeId);
if (archetypePath.empty())
    continue;
```

**After**: Replace the manifest lookup with a convention function. Add this to `SceneImportService.h` or as a local helper in `GameWorld.cpp`:

```cpp
std::string ResolvePrefabPath(const std::string& aTypeId)
{
    return "Objects/" + aTypeId + ".tgo";
}
```

**Changes in `GameWorld::BuildSceneObjects`**:

```cpp
// Before
const std::string archetypePath = myObjectManifest.Resolve(sceneObject.typeId);
if (archetypePath.empty())
    continue;

// After
const std::string prefabPath = ResolvePrefabPath(sceneObject.typeId);
```

**Changes in `GameWorld::Init`**:

Remove:
```cpp
myObjectManifest.Load("objects.manifest");
myObjectManifest.ValidateAll();
```

**Changes in `GameWorld.h`**:

Remove:
```cpp
ObjectManifest myObjectManifest;
```

**Filesystem change**: Flatten the `.tgo` directory or encode the subfolder in the typeId.

Option A — flat folder (recommended for ~20 files):
```
Objects/
    enemy_melee_basic.tgo
    enemy_melee_fast.tgo
    ground_corner_ne.tgo
    wall_long.tgo
    checkpoint_basic.tgo
    ...
```

Scene files use `typeId = enemy_melee_basic`, resolves to `Objects/enemy_melee_basic.tgo`.

Option B — subfolder in typeId:
```
Objects/
    Enemies/
        enemy_melee_basic.tgo
    Ground/
        ground_corner_ne.tgo
```

Scene files use `typeId = Enemies/enemy_melee_basic`, resolves to `Objects/Enemies/enemy_melee_basic.tgo`.

Option A is simpler. With ~20 object types there is no need for subdirectories.

**After this step**: Delete `ObjectManifest.h`, `ObjectManifest.cpp`, and the `objects.manifest` data file.

---

### Step 3: Eliminate `ResolvedObjectDef` and `ArchetypeData` — builders take `SceneObjectData`

This is the largest change but the most impactful. It makes the builders look like the old project's tag factories (see `Doc/GAMEOBJECT_SYSTEM.md`, section "Registering Tag Factories").

**Why this works**: `ResolvedObjectDef` and `SceneObjectData` are the same shape:

```
SceneObjectData:                    ResolvedObjectDef:
    name                                name
    typeId                              factoryType (was enum, now string)
    position                            position
    rotation                            rotation
    scale                               scale
    properties (map<string,any>)        properties (map<string,any>)
    TryGetProperty<T>()                 TryGetProperty<T>()
    GetPropertyOr<T>()                  GetPropertyOr<T>()
    HasProperty()                       HasProperty()
```

They have identical property maps and identical accessor methods. The only reason `ResolvedObjectDef` exists is that it held the `FactoryType` enum — which we removed in step 1.

**3a. Move prefab parsing into a free function**

Take the parsing logic from `ArchetypeLoader::Load` and make it a free function. It can live in `SceneImportService.cpp` or its own small file if preferred.

```cpp
// In SceneImportService.cpp (or PrefabParser.h/cpp if you want it separate)

struct PrefabData
{
    std::string factoryType;
    std::unordered_map<std::string, std::any> properties;
};

PrefabData ParsePrefab(const std::string& aPath);
```

The implementation is the same as `ArchetypeLoader::Load` — it detects JSON vs legacy format and parses accordingly. The only change is the return type name (`PrefabData` instead of `ArchetypeData`) and `factoryType` is just a `std::string` (not `factoryTypeString`).

**3b. Merge prefab defaults into `SceneObjectData` before dispatch**

**Current merge logic** in `GameWorld::BuildSceneObjects` (see `GameWorld.cpp`):

```cpp
const ArchetypeData archetypeData = myArchetypeLoader.Load(archetypePath);

std::unordered_map<std::string, std::any> mergedProperties = archetypeData.properties;
for (const auto& [name, value] : sceneObject.properties)
{
    mergedProperties[name] = value;
}

ResolvedObjectDef resolvedDefinition;
resolvedDefinition.factoryType = FactoryTypeFromString(archetypeData.factoryTypeString);
resolvedDefinition.name = sceneObject.name;
resolvedDefinition.position = sceneObject.position;
resolvedDefinition.rotation = sceneObject.rotation;
resolvedDefinition.scale = sceneObject.scale;
resolvedDefinition.properties = std::move(mergedProperties);

auto gameObject = GameObjectFactory::GetInstance().Build(resolvedDefinition);
```

**After** — merge directly into a copy of `SceneObjectData`:

```cpp
PrefabData prefab = ParsePrefab(prefabPath);

// Merge: prefab defaults fill in gaps, scene properties win on conflict
SceneObjectData merged = sceneObject;  // copy scene data (has overrides)
for (const auto& [name, value] : prefab.properties)
{
    if (!merged.HasProperty(name))
    {
        merged.properties[name] = value;  // only add prefab defaults that scene didn't override
    }
}

auto gameObject = GameObjectFactory::GetInstance().Build(prefab.factoryType, merged);
```

Note the merge direction is inverted compared to the current code. Currently it starts with archetype properties and overwrites with scene properties. The new version starts with scene properties and fills gaps with prefab defaults. Same result, but reads more naturally: "scene data is the base, prefab fills in what's missing."

**3c. Change builder signatures**

**Current builder** (`GameObjectFactoryRegistrations.cpp`):

```cpp
std::unique_ptr<GameObject> BuildStaticWorld(const ResolvedObjectDef& aDefinition)
{
    auto object = std::make_unique<GameObject>(aDefinition.name);
    object->SetLayer(ObjectLayer::WorldStatic);

    auto& transform = object->GetTransform();
    transform.SetPosition(aDefinition.position);
    transform.SetRotation(aDefinition.rotation);
    transform.SetScale(aDefinition.scale);

    const std::string modelPath = aDefinition.GetPropertyOr<std::string>("modelPath", "");
    if (!modelPath.empty())
    {
        object->AddComponent<MeshComponent>(modelPath);
    }

    const std::string colliderType = aDefinition.GetPropertyOr<std::string>("colliderType", "");
    if (colliderType == "box")
    {
        const auto halfExtents = aDefinition.GetPropertyOr<Vector3f>("colliderSize", Vector3f(0,0,0));
        const auto offset = aDefinition.GetPropertyOr<Vector3f>("colliderOffset", Vector3f(0,0,0));
        if (halfExtents.x > 0.0f || halfExtents.y > 0.0f || halfExtents.z > 0.0f)
        {
            object->AddComponent<BoxColliderComponent>(size, offset);
        }
    }

    return object;
}
```

**After** — takes `SceneObjectData`, no transform boilerplate (transform is applied by `BuildSceneObjects` after the builder returns):

```cpp
std::unique_ptr<GameObject> BuildStaticWorld(const SceneObjectData& aData)
{
    auto object = std::make_unique<GameObject>(aData.name);
    object->SetLayer(ObjectLayer::WorldStatic);

    const std::string modelPath = aData.GetPropertyOr<std::string>("modelPath", "");
    if (!modelPath.empty())
    {
        object->AddComponent<MeshComponent>(modelPath);
    }

    const std::string colliderType = aData.GetPropertyOr<std::string>("colliderType", "");
    if (colliderType == "box")
    {
        const auto size = aData.GetPropertyOr<Vector3f>("colliderSize", Vector3f(0,0,0));
        const auto offset = aData.GetPropertyOr<Vector3f>("colliderOffset", Vector3f(0,0,0));
        if (size.x > 0.0f || size.y > 0.0f || size.z > 0.0f)
        {
            object->AddComponent<BoxColliderComponent>(size, offset);
        }
    }

    return object;
}
```

Two changes from the current builder: `const ResolvedObjectDef&` → `const SceneObjectData&`, and the transform block is gone. The transform is always the same for every object, so `BuildSceneObjects` applies it once after the builder returns — builders never touch it.

**3d. Update `GameObjectFactory` types**

```cpp
// GameObjectFactory.h — final form
class GameObjectFactory
{
public:
    using FactoryFunction = std::function<std::unique_ptr<GameObject>(const SceneObjectData&)>;

    void Register(const std::string& aType, FactoryFunction aFactory);
    std::unique_ptr<GameObject> Build(const std::string& aFactoryType,
                                      const SceneObjectData& aData) const;
    static GameObjectFactory& GetInstance();

private:
    std::unordered_map<std::string, FactoryFunction> myFactories;
};
```

**After this step**: Delete `ResolvedObjectDef.h`, `ArchetypeLoader.h`, `ArchetypeLoader.cpp`. Remove `ArchetypeLoader myArchetypeLoader` from `GameWorld.h`.

---

### Step 4: Simplify `BuildSceneObjects`

After steps 1–3, the entire function collapses.

**Current implementation** (`GameWorld.cpp`, ~40 lines with error handling):

```cpp
std::vector<std::unique_ptr<GameObject>> GameWorld::BuildSceneObjects(
    const std::vector<SceneObjectData>& someSceneObjects)
{
    std::vector<std::unique_ptr<GameObject>> objects;
    objects.reserve(someSceneObjects.size());

    for (const SceneObjectData& sceneObject : someSceneObjects)
    {
        if (sceneObject.typeId.empty())
            continue;

        const std::string archetypePath = myObjectManifest.Resolve(sceneObject.typeId);
        if (archetypePath.empty())
            continue;

        try
        {
            const ArchetypeData archetypeData = myArchetypeLoader.Load(archetypePath);

            std::unordered_map<std::string, std::any> mergedProperties = archetypeData.properties;
            for (const auto& [name, value] : sceneObject.properties)
            {
                mergedProperties[name] = value;
            }

            ResolvedObjectDef resolvedDefinition;
            resolvedDefinition.factoryType = FactoryTypeFromString(archetypeData.factoryTypeString);
            resolvedDefinition.name = sceneObject.name;
            resolvedDefinition.position = sceneObject.position;
            resolvedDefinition.rotation = sceneObject.rotation;
            resolvedDefinition.scale = sceneObject.scale;
            resolvedDefinition.properties = std::move(mergedProperties);

            auto gameObject = GameObjectFactory::GetInstance().Build(resolvedDefinition);
            if (gameObject)
                objects.push_back(std::move(gameObject));
        }
        catch (const std::exception& exception)
        {
            ERROR_PRINT(exception.what());
        }
    }

    return objects;
}
```

**After refactor**:

```cpp
std::vector<std::unique_ptr<GameObject>> GameWorld::BuildSceneObjects(
    const std::vector<SceneObjectData>& someSceneObjects)
{
    std::vector<std::unique_ptr<GameObject>> objects;
    objects.reserve(someSceneObjects.size());

    for (const SceneObjectData& sceneObject : someSceneObjects)
    {
        if (sceneObject.typeId.empty())
            continue;

        try
        {
            // 1. Load prefab defaults
            const std::string prefabPath = "Objects/" + sceneObject.typeId + ".tgo";
            PrefabData prefab = ParsePrefab(prefabPath);

            // 2. Merge: scene overrides win, prefab fills gaps
            SceneObjectData merged = sceneObject;
            for (const auto& [name, value] : prefab.properties)
            {
                if (!merged.HasProperty(name))
                    merged.properties[name] = value;
            }

            // 3. Build and apply transform
            auto gameObject = GameObjectFactory::GetInstance().Build(prefab.factoryType, merged);
            if (gameObject)
            {
                auto& transform = gameObject->GetTransform();
                transform.SetPosition(merged.position);
                transform.SetRotation(merged.rotation);
                transform.SetScale(merged.scale);

                objects.push_back(std::move(gameObject));
            }
        }
        catch (const std::exception& e)
        {
            ERROR_PRINT("Failed to build '%s' (typeId: %s): %s",
                        sceneObject.name.c_str(), sceneObject.typeId.c_str(), e.what());
        }
    }

    return objects;
}
```

No manifest. No archetype loader class. No intermediate types. No enum conversion.

---

## Complete before/after: adding a new object type

### Before (7 steps)

Documented in `Object_System_Guide_neutral.md`, section "How To Add a New Object Type":

1. Add enum value in `FactoryType.h`
2. Add string-to-enum mapping in `FactoryTypeFromString()` in `GameObjectFactory.cpp`
3. Write builder function in `GameObjectFactoryRegistrations.cpp`
4. Register builder in `RegisterGameObjectFactories()`
5. Create `.tgo` prefab file
6. Add entry in `objects.manifest`
7. Place in scene with matching `typeId`

### After (3 steps)

1. **Write builder + register it** in `GameObjectFactoryRegistrations.cpp`
2. **Create `.tgo` prefab file** at `Objects/{typeId}.tgo`
3. **Place in scene** with matching `typeId`

### Full example: adding a Turret enemy

**Step 1** — Builder and registration (`GameObjectFactoryRegistrations.cpp`):

```cpp
std::unique_ptr<GameObject> BuildEnemyTurret(const SceneObjectData& aData)
{
    auto obj = std::make_unique<GameObject>(aData.name);
    obj->SetLayer(ObjectLayer::Enemy);

    const std::string modelPath = aData.GetPropertyOr<std::string>("modelPath", "");
    if (!modelPath.empty())
    {
        obj->AddComponent<MeshComponent>(modelPath);
    }

    obj->AddComponent<CapsuleColliderComponent>(
        aData.GetPropertyOr<float>("colliderRadius", 0.5f),
        aData.GetPropertyOr<float>("colliderHeight", 1.0f),
        aData.GetPropertyOr<Vector3f>("colliderOffset", Vector3f(0, 0.5f, 0)));

    obj->AddComponent<HealthComponent>(aData.GetPropertyOr<int>("maxHealth", 5));
    obj->AddComponent<HurtboxComponent>(ObjectLayer::Enemy);
    obj->AddComponent<TurretController>(
        aData.GetPropertyOr<float>("rotateSpeed", 2.0f),
        aData.GetPropertyOr<float>("fireRate", 1.5f),
        aData.GetPropertyOr<int>("projectileDamage", 1));

    // Register with encounter system if applicable
    std::string encounterId;
    if (aData.TryGetProperty("encounterId", encounterId))
    {
        obj->AddComponent<EnemyComponent>(encounterId, ObjectLayer::Enemy);
    }

    return obj;
}

void RegisterGameObjectFactories()
{
    GameObjectFactory& factory = GameObjectFactory::GetInstance();
    factory.Register("StaticWorld",   BuildStaticWorld);
    factory.Register("EnemyMelee",   BuildEnemyMelee);
    factory.Register("EnemyRoll",    BuildEnemyRoll);
    factory.Register("EnemyTurret",  BuildEnemyTurret);   // new
    factory.Register("Checkpoint",   BuildCheckpoint);
    factory.Register("Door",         BuildDoor);
    // ...
}
```

**Step 2** — Prefab file (`Objects/enemy_turret_basic.tgo`):

```
factoryType     = EnemyTurret
modelPath       = Models/Enemies/turret_basic.fbx
colliderRadius  = 0.5
colliderHeight  = 1.0
colliderOffset  = (0, 0.5, 0)
maxHealth       = 5
rotateSpeed     = 2.0
fireRate        = 1.5
projectileDamage = 1
```

**Step 3** — Place in scene with `typeId = enemy_turret_basic`.

A stat variant (e.g., a fast-firing turret) only needs a new `.tgo` file:

```
# Objects/enemy_turret_fast.tgo
factoryType     = EnemyTurret
modelPath       = Models/Enemies/turret_fast.fbx
maxHealth       = 3
rotateSpeed     = 4.0
fireRate        = 0.5
projectileDamage = 1
```

Place with `typeId = enemy_turret_fast`. Same builder, different defaults. No code changes.

---

## `.tgo` prefab file specification

### Required properties

Every `.tgo` file must have exactly one required property:

| Property | Type | Purpose |
|---|---|---|
| `factoryType` | `string` | Selects which builder function constructs this object. Must match a string registered via `factory.Register(...)` in `GameObjectFactoryRegistrations.cpp`. If missing, `ParsePrefab` fails and the object is not created. |

That is the only property the pipeline itself requires. Everything else is consumed by the builder function that `factoryType` selects.

### Common properties used by builders

These are not enforced by the pipeline — they are conventions used across most builder functions. Builders access them via `aData.GetPropertyOr<T>(name, default)`, so missing properties silently fall back to the default value specified in the builder code.

#### Visual

| Property | Type | Used by | Purpose |
|---|---|---|---|
| `modelPath` | `string` | All builders with a visual representation | Path to `.fbx` model file. If empty, no `MeshComponent` is added. |

#### Collision

| Property | Type | Used by | Purpose |
|---|---|---|---|
| `colliderType` | `string` | `StaticWorld`, enemies, `Player` | `"box"` or `"capsule"`. Determines which collider component is added. If empty, no collider. |
| `colliderSize` | `Vector3f` | `StaticWorld` (box colliders) | Full size of the box collider volume (width, height, depth). |
| `colliderOffset` | `Vector3f` | `StaticWorld`, enemies, `Player` | Offset of the collider center from the object origin. |
| `colliderRadius` | `float` | Enemies, `Player` (capsule colliders) | Radius of the capsule collider. |
| `colliderHeight` | `float` | Enemies, `Player` (capsule colliders) | Height of the capsule collider. |

#### Combat and gameplay

| Property | Type | Used by | Purpose |
|---|---|---|---|
| `maxHealth` | `int` | Enemies, `Player` | Starting and maximum HP for `HealthComponent`. |
| `damage` | `int` | Enemies | Base damage dealt by this enemy's attacks. |
| `moveSpeed` | `float` | Enemies, `Player` | Movement speed in world units per second. |
| `aggroRange` | `float` | `EnemyMelee` | Distance at which the enemy begins chasing the player. |
| `attackRange` | `float` | `EnemyMelee` | Distance at which the enemy can initiate an attack. |
| `windupTime` | `float` | `EnemyMelee` | Duration of the attack windup animation before the hit is active. |
| `recoveryTime` | `float` | `EnemyMelee` | Cooldown duration after an attack before the enemy can act again. |
| `knockbackResistance` | `float` | `EnemyMelee` | Resistance to knockback impulses. `0.0` = full knockback, `1.0` = immune. |
| `dodgeSpeed` | `float` | `Player` | Speed during the dodge roll. |

#### Encounter and world systems

| Property | Type | Used by | Purpose |
|---|---|---|---|
| `encounterId` | `string` | Enemies, `Door`, `EncounterTrigger` | Links the object to an encounter group. Set per-instance in the scene, not in the prefab. |
| `checkpointId` | `string` | `Checkpoint` | Unique identifier for checkpoint registration. Set per-instance in the scene. |
| `targetScene` | `string` | `Door` | Scene to load when the door is used for level transition. |
| `targetSpawnId` | `string` | `Door` | Spawn marker ID in the target scene. |

#### Rolling enemy specific

| Property | Type | Used by | Purpose |
|---|---|---|---|
| `rollSpeed` | `float` | `EnemyRoll` | Speed during the roll attack. |
| `aimTime` | `float` | `EnemyRoll` | Duration the enemy spends aiming before rolling. |
| `stunDuration` | `float` | `EnemyRoll` | How long the enemy is stunned after hitting a wall. |
| `recoverTime` | `float` | `EnemyRoll` | Recovery time after a roll completes. |

### Prefab vs scene-instance properties

Some properties belong in the `.tgo` prefab (shared defaults), others belong on the scene instance (per-placement data).

**Belongs in the prefab** — anything that defines what this *type* of object is:
- `factoryType`, `modelPath`, `colliderType`, collider dimensions
- `maxHealth`, `damage`, `moveSpeed`, `aggroRange` — stat defaults
- Any property that is the same for every instance of this type

**Belongs on the scene instance** — anything that varies per placement:
- `encounterId` — which encounter group this specific enemy is in
- `checkpointId` — unique ID for this specific checkpoint
- `targetScene`, `targetSpawnId` — where this specific door leads
- Stat overrides — e.g., one enemy in a room has `damage = 2` instead of the default

The merge rule is: scene instance values win over prefab defaults. If a property exists on both, the scene value is used. This lets you override any default on a per-instance basis without creating a new `.tgo` file.

### Supported value types

The `.tgo` parser (legacy key-value format) resolves values in this order:

| Syntax | Parsed as | C++ type |
|---|---|---|
| `(x, y, z)` | Vector3 | `CommonUtilities::Vector3<float>` |
| `true` / `false` | Boolean | `bool` |
| `42` (no decimal point) | Integer | `int` |
| `3.14` (has decimal point) | Float | `float` |
| Anything else | String | `std::string` |

All values are stored as `std::any` in the property map. Builders retrieve them with `GetPropertyOr<T>()` which performs `std::any_cast<T>` — so the type in the `.tgo` must match the type the builder expects. For example, if a builder calls `GetPropertyOr<float>("moveSpeed", 3.0f)`, the `.tgo` value must parse as a float (write `3.0`, not `3`).

### Minimal `.tgo` examples

Static prop (no collider):
```
factoryType = Decoration
modelPath   = Models/Props/barrel.fbx
```

Static world piece (with collider):
```
factoryType        = StaticWorld
modelPath          = Models/Ground/floor_tile.fbx
colliderType       = box
colliderSize    = (200, 40, 200)
colliderOffset  = (0, -20, 0)
```

Enemy variant:
```
factoryType         = EnemyMelee
modelPath           = Models/Enemies/melee_fast.fbx
colliderRadius      = 0.4
colliderHeight      = 1.8
colliderOffset      = (0, 0.9, 0)
maxHealth           = 2
damage              = 1
moveSpeed           = 4.4
aggroRange          = 7.0
attackRange         = 1.1
windupTime          = 0.2
recoveryTime        = 0.5
knockbackResistance = 0.0
```

---

## Two design questions answered

### When do I need a new builder vs a new `.tgo`?

**New `.tgo` only** (no code): The object needs the same set of components but with different stat values. Examples: fast melee enemy vs tank melee enemy, small health pickup vs large health pickup.

**New builder + new `.tgo`** (code change): The object needs a different set of components. Examples: a turret enemy that has no movement but has targeting, a destructible prop that has health but no AI.

### What is the `factoryType` string in the `.tgo`?

It is the dispatch key that selects which builder function constructs the object. It answers: "what components does this object need?" The string must exactly match the string used in `factory.Register(...)` in `GameObjectFactoryRegistrations.cpp`.

Current factory types (from `FactoryType.h`, to be used as string keys):

```
"StaticWorld"
"Player"
"EnemyMelee"
"EnemyRoll"
"EnemyRanged"
"Checkpoint"
"Door"
"SpawnMarker"
"EncounterTrigger"
"PickupHealth"
"NpcHub"
"DeathVolume"
"Decoration"
```

---

## Migration order

These steps can each be merged independently. They do not need to happen in one PR.

| Order | Step | Risk | Touches |
|---|---|---|---|
| 1 | Enum to string dispatch | Low | `GameObjectFactory.h/.cpp`, `GameObjectFactoryRegistrations.cpp`, `GameWorld.cpp` |
| 2 | Kill manifest | Low | `GameWorld.h/.cpp`, filesystem (move `.tgo` files) |
| 3 | Eliminate `ResolvedObjectDef` + `ArchetypeData` | Medium | `GameObjectFactory.h/.cpp`, `GameObjectFactoryRegistrations.cpp`, `GameWorld.h/.cpp`, `SceneImportService.cpp` |
| 4 | Simplify `BuildSceneObjects` | Low | `GameWorld.cpp` (cleanup after step 3) |

Steps 1 and 2 are independent and can be done in parallel on separate branches.
Step 3 depends on step 1 (needs string dispatch in place).
Step 4 is cleanup after step 3.

After all steps, delete:
- `FactoryType.h`
- `ResolvedObjectDef.h`
- `ObjectManifest.h` / `ObjectManifest.cpp`
- `ArchetypeLoader.h` / `ArchetypeLoader.cpp`
- `objects.manifest`

---

## Reference files

Current implementation:
- `Source/Game/source/GameObjectFactory.h` / `.cpp` — factory dispatch
- `Source/Game/source/GameObjectFactoryRegistrations.cpp` — builder functions
- `Source/Game/source/FactoryType.h` — enum (to be removed)
- `Source/Game/source/ResolvedObjectDef.h` — intermediate type (to be removed)
- `Source/Game/source/ArchetypeLoader.h` / `.cpp` — prefab parser (to be inlined)
- `Source/Game/source/ObjectManifest.h` / `.cpp` — manifest (to be removed)
- `Source/Game/source/SceneObjectData.h` — scene data struct (kept, becomes the single data type)
- `Source/Game/source/SceneImportService.h` / `.cpp` — scene loader
- `Source/Game/source/GameWorld.h` / `.cpp` — orchestration
- `Source/Game/source/GameObject.h` — game object class

Design context:
- `Doc/p4_system_design.md` — game systems reference (sections 1–2 cover current pipeline)
- `Doc/GAMEOBJECT_SYSTEM.md` — old project's tag-factory pattern (section "Registering Tag Factories")
- `Doc/Object_System_Guide_neutral.md` — current system documentation (to be updated after refactor)
