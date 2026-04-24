#include "VfxSystem.h"

#include "DebugSettings.h"

#include <tge/drawers/LineDrawer.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/engine.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/math/Matrix4x4.h>
#include <tge/primitives/LinePrimitive.h>
#include <tge/settings/Settings.h>
#include <tge/sprite/sprite.h>
#include <tge/texture/TextureManager.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>

namespace
{
    using Json = nlohmann::json;
    constexpr const char* kDefaultVfxTextureFolder = "Animations/VFX/";

    float ClampPositive(const float aValue, const float aFallback)
    {
        return aValue > 0.0f ? aValue : aFallback;
    }

    int ClampPositive(const int aValue, const int aFallback)
    {
        return aValue > 0 ? aValue : aFallback;
    }

    VfxSystem::Space ParseSpace(const std::string& aSpace)
    {
        std::string lowered = aSpace;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lowered == "screen")
        {
            return VfxSystem::Space::Screen;
        }

        return VfxSystem::Space::World;
    }

    VfxSystem::ParticleMotionMode ParseParticleMotionMode(const std::string& aMode)
    {
        std::string lowered = aMode;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lowered == "falling")
        {
            return VfxSystem::ParticleMotionMode::Falling;
        }

        if (lowered == "floating")
        {
            return VfxSystem::ParticleMotionMode::Floating;
        }

        return VfxSystem::ParticleMotionMode::Static;
    }

    CommonUtilities::Vector2<float> ParseVector2(const Json& aJsonValue, const CommonUtilities::Vector2<float>& aFallback)
    {
        if (!aJsonValue.is_array() || aJsonValue.size() < 2)
        {
            return aFallback;
        }

        if (!aJsonValue[0].is_number() || !aJsonValue[1].is_number())
        {
            return aFallback;
        }

        return {
            aJsonValue[0].get<float>(),
            aJsonValue[1].get<float>()
        };
    }

    CommonUtilities::Vector3<float> ParseVector3(const Json& aJsonValue, const CommonUtilities::Vector3<float>& aFallback)
    {
        if (!aJsonValue.is_array() || aJsonValue.size() < 3)
        {
            return aFallback;
        }

        if (!aJsonValue[0].is_number() || !aJsonValue[1].is_number() || !aJsonValue[2].is_number())
        {
            return aFallback;
        }

        return {
            aJsonValue[0].get<float>(),
            aJsonValue[1].get<float>(),
            aJsonValue[2].get<float>()
        };
    }

    Tga::Color ParseColor(const Json& aJsonValue, const Tga::Color& aFallback)
    {
        if (!aJsonValue.is_array() || aJsonValue.size() < 4)
        {
            return aFallback;
        }

        if (!aJsonValue[0].is_number() || !aJsonValue[1].is_number() || !aJsonValue[2].is_number() || !aJsonValue[3].is_number())
        {
            return aFallback;
        }

        return {
            aJsonValue[0].get<float>(),
            aJsonValue[1].get<float>(),
            aJsonValue[2].get<float>(),
            aJsonValue[3].get<float>()
        };
    }

    std::string NormalizeTexturePath(const std::string& aTexturePath)
    {
        if (aTexturePath.empty())
        {
            return {};
        }

        if (aTexturePath.find('/') == std::string::npos &&
            aTexturePath.find('\\') == std::string::npos)
        {
            return std::string(kDefaultVfxTextureFolder) + aTexturePath;
        }

        return aTexturePath;
    }

    struct FrameSample
    {
        int globalFrameIndex = 0;
        Tga::TextureRext textureRect = { 0.0f, 0.0f, 1.0f, 1.0f };
    };

    FrameSample ComputeFrameSample(
        const int aFrameCount,
        const float aFps,
        const bool aLoop,
        const int aDefaultColumns,
        const int aDefaultRows,
        const float anAgeSeconds)
    {
        const int columns = ClampPositive(aDefaultColumns, 1);
        const int rows = ClampPositive(aDefaultRows, 1);
        const int sheetCapacity = (columns * rows > 1) ? (columns * rows) : 1;

        const int frameCount = ClampPositive(aFrameCount, 1);
        const int effectiveFrameCount = (std::min)(frameCount, sheetCapacity);

        const float clampedAgeSeconds = anAgeSeconds > 0.0f ? anAgeSeconds : 0.0f;
        int frameIndex = static_cast<int>(std::floor(clampedAgeSeconds * ClampPositive(aFps, 1.0f)));
        if (aLoop)
        {
            frameIndex %= effectiveFrameCount;
        }
        else
        {
            frameIndex = std::clamp(frameIndex, 0, effectiveFrameCount - 1);
        }

        const int localFrameIndex = std::clamp(frameIndex, 0, sheetCapacity - 1);
        const int column = localFrameIndex % columns;
        const int row = std::clamp(localFrameIndex / columns, 0, rows - 1);

        const float u0 = static_cast<float>(column) / static_cast<float>(columns);
        const float v0 = static_cast<float>(row) / static_cast<float>(rows);
        const float u1 = static_cast<float>(column + 1) / static_cast<float>(columns);
        const float v1 = static_cast<float>(row + 1) / static_cast<float>(rows);

        return {
            frameIndex,
            { u0, v0, u1, v1 }
        };
    }

}

VfxSystem* VfxService::ourSystem = nullptr;

VfxSystem::VfxSystem()
{
    myActiveEffects.resize(kMaxActiveEffects);
}

bool VfxSystem::Init()
{
    return LoadEffectDefinitions();
}

void VfxSystem::ClearActiveEffects()
{
    for (ActiveEffect& effect : myActiveEffects)
    {
        effect = {};
    }
}

void VfxSystem::Update(const float aDeltaTime)
{
    const float safeDeltaTime = aDeltaTime > 0.0f ? aDeltaTime : 0.0f;

    for (ActiveEffect& effect : myActiveEffects)
    {
        if (!effect.isActive || !effect.definition)
        {
            continue;
        }

        effect.ageSeconds += safeDeltaTime;

        if (effect.definition->space == Space::World && safeDeltaTime > 0.0f)
        {
            effect.runtimeVelocity.x += effect.definition->acceleration.x * safeDeltaTime;
            effect.runtimeVelocity.y += effect.definition->acceleration.y * safeDeltaTime;
            effect.runtimeVelocity.z += effect.definition->acceleration.z * safeDeltaTime;

            effect.runtimeWorldPosition.x += effect.runtimeVelocity.x * safeDeltaTime;
            effect.runtimeWorldPosition.y += effect.runtimeVelocity.y * safeDeltaTime;
            effect.runtimeWorldPosition.z += effect.runtimeVelocity.z * safeDeltaTime;

            if (effect.definition->motionMode == ParticleMotionMode::Falling || effect.definition->motionMode == ParticleMotionMode::Floating)
            {
                const float safeDriftFrequency = effect.definition->driftFrequency > 0.0f ? effect.definition->driftFrequency : 1.0f;
                const float driftWave = std::sin(effect.ageSeconds * safeDriftFrequency + effect.driftPhase);
                effect.runtimeWorldPosition.x += driftWave * effect.definition->driftStrength * safeDeltaTime;
            }

            if (effect.definition->motionMode == ParticleMotionMode::Floating)
            {
                const float safeDriftFrequency = effect.definition->driftFrequency > 0.0f ? effect.definition->driftFrequency : 1.0f;
                const float verticalWave = std::cos(effect.ageSeconds * safeDriftFrequency * 0.75f + effect.driftPhase * 0.8f);
                effect.runtimeWorldPosition.y += verticalWave * effect.definition->driftStrength * safeDeltaTime * 0.65f;
            }
        }

        if (effect.definition->debugLogFrames && effect.texture)
        {
            const FrameSample debugFrameSample = ComputeFrameSample(
                effect.definition->frameCount,
                effect.definition->fps,
                effect.definition->loop,
                effect.definition->columns,
                effect.definition->rows,
                effect.ageSeconds);

            if (debugFrameSample.globalFrameIndex != effect.lastDebugFrameIndex)
            {
                effect.lastDebugFrameIndex = debugFrameSample.globalFrameIndex;
                std::cout
                    << "[VFX] '" << effect.definition->id
                    << "' frame=" << debugFrameSample.globalFrameIndex
                    << " age=" << effect.ageSeconds
                    << "s\n";
            }
        }

        const float durationSeconds = CalculateDurationSeconds(*effect.definition);
        if (!effect.definition->loop && durationSeconds > 0.0f && effect.ageSeconds >= durationSeconds)
        {
            effect = {};
        }
        else if (effect.definition->loop && effect.definition->lifetimeSeconds > 0.0f && effect.ageSeconds >= effect.definition->lifetimeSeconds)
        {
            effect = {};
        }
    }
}

void VfxSystem::Render()
{
    Tga::Engine* engine = Tga::Engine::GetInstance();
    if (!engine)
    {
        return;
    }

    Tga::SpriteDrawer& spriteDrawer = engine->GetGraphicsEngine().GetSpriteDrawer();
    Tga::LineDrawer& lineDrawer = engine->GetGraphicsEngine().GetLineDrawer();
    auto& graphicsStateStack = engine->GetGraphicsEngine().GetGraphicsStateStack();
    const bool showVfxDebugLines = GameDebugSettings::ShowVfxDebugLines();
    const bool showVfxPivotMarker = GameDebugSettings::ShowVfxPivotMarker();

    struct ScreenTextureBuckets
    {
        Tga::SpriteSharedData sharedData;
        std::vector<Tga::Sprite2DInstanceData> screenInstances;
    };

    struct WorldSprite
    {
        Tga::SpriteSharedData sharedData;
        Tga::Sprite3DInstanceData instance;
        float depthZ = 0.0f;
    };

    std::vector<WorldSprite> worldSprites;
    worldSprites.reserve(myActiveEffects.size());

    std::unordered_map<const Tga::Texture*, ScreenTextureBuckets> screenBuckets;
    screenBuckets.reserve(16);

    for (const ActiveEffect& effect : myActiveEffects)
    {
        if (!effect.isActive || !effect.definition || !effect.texture)
        {
            continue;
        }

        const FrameSample frameSample = ComputeFrameSample(
            effect.definition->frameCount,
            effect.definition->fps,
            effect.definition->loop,
            effect.definition->columns,
            effect.definition->rows,
            effect.ageSeconds);
        const Tga::Texture* texture = effect.texture;
        Tga::TextureRext textureRect = frameSample.textureRect;

        if (effect.definition->space == Space::World)
        {
            Tga::Sprite3DInstanceData instance;
            const float safeSizeMultiplier = effect.params.sizeMultiplier > 0.01f ? effect.params.sizeMultiplier : 0.01f;
            const float sizeX = (effect.definition->size.x > 0.01f ? effect.definition->size.x : 0.01f) * safeSizeMultiplier;
            const float sizeY = (effect.definition->size.y > 0.01f ? effect.definition->size.y : 0.01f) * safeSizeMultiplier;

            CommonUtilities::Vector3<float> offset = effect.definition->spawnOffsetWorld;
            if (effect.definition->useOwnerForward)
            {
                offset.x *= effect.params.ownerForwardSign;
            }

            const CommonUtilities::Vector3<float> finalPosition(
                effect.runtimeWorldPosition.x + offset.x,
                effect.runtimeWorldPosition.y + offset.y,
                effect.runtimeWorldPosition.z + offset.z);

            // Sprite3D quad vertices are corner-anchored in local space.
            // Apply pivot correction on X so left/right placement mirrors predictably,
            // while preserving legacy authored Y behavior.
            const CommonUtilities::Vector3<float> spriteOriginPosition(
                finalPosition.x - (effect.definition->pivot.x * sizeX),
                finalPosition.y,
                finalPosition.z);

            if (effect.definition->useOwnerForward && effect.params.ownerForwardSign < 0.0f)
            {
                std::swap(textureRect.myStartX, textureRect.myEndX);
            }

            instance.myTransform = Tga::Matrix4x4f::CreateFromScale({ sizeX, sizeY, 1.0f });
            instance.myTransform.SetPosition({
                spriteOriginPosition.x,
                spriteOriginPosition.y,
                spriteOriginPosition.z
            });
            instance.myColor = effect.params.color;
            instance.myTextureRect = textureRect;

            if (showVfxDebugLines)
            {
                auto drawLine = [&](const CommonUtilities::Vector3<float>& aFrom, const CommonUtilities::Vector3<float>& aTo, const Tga::Color& aColor)
                {
                    Tga::LinePrimitive line;
                    line.fromPosition = { aFrom.x, aFrom.y, aFrom.z };
                    line.toPosition = { aTo.x, aTo.y, aTo.z };
                    line.color = aColor.AsVec4();
                    lineDrawer.Draw(line);
                };

                const CommonUtilities::Vector3<float> ownerPosition = effect.runtimeWorldPosition;
                const float dominantSize = sizeX > sizeY ? sizeX : sizeY;
                const float markerExtent = dominantSize * 0.1f > 10.0f ? dominantSize * 0.1f : 10.0f;

                drawLine(ownerPosition, finalPosition, { 0.2f, 0.8f, 1.0f, 1.0f });

                drawLine(
                    { ownerPosition.x - markerExtent, ownerPosition.y, ownerPosition.z },
                    { ownerPosition.x + markerExtent, ownerPosition.y, ownerPosition.z },
                    { 1.0f, 1.0f, 0.0f, 1.0f });
                drawLine(
                    { ownerPosition.x, ownerPosition.y - markerExtent, ownerPosition.z },
                    { ownerPosition.x, ownerPosition.y + markerExtent, ownerPosition.z },
                    { 1.0f, 1.0f, 0.0f, 1.0f });

                drawLine(
                    { finalPosition.x - markerExtent, finalPosition.y, finalPosition.z },
                    { finalPosition.x + markerExtent, finalPosition.y, finalPosition.z },
                    { 0.1f, 1.0f, 0.1f, 1.0f });
                drawLine(
                    { finalPosition.x, finalPosition.y - markerExtent, finalPosition.z },
                    { finalPosition.x, finalPosition.y + markerExtent, finalPosition.z },
                    { 0.1f, 1.0f, 0.1f, 1.0f });

                if (showVfxPivotMarker)
                {
                    const float pivotExtent = dominantSize * 0.06f > 6.0f ? dominantSize * 0.06f : 6.0f;
                    drawLine(
                        { finalPosition.x - pivotExtent, finalPosition.y, finalPosition.z },
                        { finalPosition.x + pivotExtent, finalPosition.y, finalPosition.z },
                        { 1.0f, 0.25f, 0.9f, 1.0f });
                    drawLine(
                        { finalPosition.x, finalPosition.y - pivotExtent, finalPosition.z },
                        { finalPosition.x, finalPosition.y + pivotExtent, finalPosition.z },
                        { 1.0f, 0.25f, 0.9f, 1.0f });
                }
            }

            Tga::SpriteSharedData sharedData;
            sharedData.myTexture = texture;

            worldSprites.push_back({
                sharedData,
                instance,
                finalPosition.z
                });
            continue;
        }

        ScreenTextureBuckets& bucket = screenBuckets[texture];
        bucket.sharedData.myTexture = texture;

        Tga::Sprite2DInstanceData instance;
        instance.myPosition = {
            effect.params.screenPosition.x + effect.definition->spawnOffsetScreen.x,
            effect.params.screenPosition.y + effect.definition->spawnOffsetScreen.y
        };
        instance.myPivot = {
            effect.definition->pivot.x,
            effect.definition->pivot.y
        };
        instance.mySize = {
            effect.definition->size.x,
            effect.definition->size.y
        };
        instance.mySizeMultiplier = {
            effect.params.sizeMultiplier,
            effect.params.sizeMultiplier
        };
        instance.myRotation = effect.params.rotationRadians;
        instance.myColor = effect.params.color;
        instance.myTextureRect = textureRect;

        bucket.screenInstances.push_back(instance);
    }

    std::vector<const WorldSprite*> sortedWorldSprites;
    sortedWorldSprites.reserve(worldSprites.size());
    for (const WorldSprite& worldSprite : worldSprites)
    {
        sortedWorldSprites.push_back(&worldSprite);
    }

    std::stable_sort(
        sortedWorldSprites.begin(),
        sortedWorldSprites.end(),
        [](const WorldSprite* aLeft, const WorldSprite* aRight)
        {
            return aLeft->depthZ > aRight->depthZ;
        });

    for (const WorldSprite* sprite : sortedWorldSprites)
    {
        spriteDrawer.Draw(sprite->sharedData, sprite->instance);
    }

    for (auto& [texture, bucket] : screenBuckets)
    {
        (void)texture;

        if (!bucket.screenInstances.empty())
        {
            graphicsStateStack.Push();
            graphicsStateStack.SetDefaultCamera();
            spriteDrawer.Draw(bucket.sharedData, bucket.screenInstances.data(), bucket.screenInstances.size());
            graphicsStateStack.Pop();
        }
    }
}

bool VfxSystem::SpawnEffect(const std::string& anEffectId, const SpawnParams& someParams)
{
    const auto definitionIt = myDefinitions.find(anEffectId);
    if (definitionIt == myDefinitions.end())
    {
        return false;
    }

    Tga::Engine* engine = Tga::Engine::GetInstance();
    if (!engine)
    {
        return false;
    }

    Tga::Texture* texture = engine->GetTextureManager().GetTexture(definitionIt->second.texturePath.c_str());
    if (!texture)
    {
        std::cout << "[VFX] Missing texture for effect '" << definitionIt->second.id << "': " << definitionIt->second.texturePath << "\n";
        return false;
    }

    for (ActiveEffect& effect : myActiveEffects)
    {
        if (effect.isActive)
        {
            continue;
        }

        effect.isActive = true;
        effect.definition = &definitionIt->second;
        effect.texture = texture;
        effect.params = someParams;
        if (!effect.params.useCustomColor)
        {
            effect.params.color = definitionIt->second.color;
        }
        effect.ageSeconds = 0.0f;
        effect.lastDebugFrameIndex = -1;
        effect.runtimeWorldPosition = effect.params.worldPosition;
        effect.runtimeVelocity = definitionIt->second.initialVelocity;

        static std::mt19937 randomGenerator{ std::random_device{}() };
        std::uniform_real_distribution<float> driftPhaseDistribution(0.0f, 6.2831853f);
        effect.driftPhase = driftPhaseDistribution(randomGenerator);

        if (effect.definition->debugLogFrames)
        {
            std::cout << "[VFX] Spawned debug-tracked effect '" << effect.definition->id << "'\n";
        }
        return true;
    }

    return false;
}

bool VfxSystem::SpawnWorldEffect(const std::string& anEffectId, const CommonUtilities::Vector3<float>& aPosition, const float aSizeMultiplier, const float anOwnerForwardSign)
{
    SpawnParams params;
    params.worldPosition = aPosition;
    params.sizeMultiplier = aSizeMultiplier;
    params.ownerForwardSign = anOwnerForwardSign;
    return SpawnEffect(anEffectId, params);
}

bool VfxSystem::SpawnScreenEffect(const std::string& anEffectId, const CommonUtilities::Vector2<float>& aPosition, const float aSizeMultiplier)
{
    SpawnParams params;
    params.screenPosition = aPosition;
    params.sizeMultiplier = aSizeMultiplier;
    return SpawnEffect(anEffectId, params);
}

void VfxSystem::BeginSceneTransition(const std::string& aFromScene, const std::string& aToScene)
{
    (void)aFromScene;
    (void)aToScene;
    ClearActiveEffects();
}

bool VfxSystem::ReloadDefinitions()
{
    return LoadEffectDefinitions();
}

int VfxSystem::GetActiveCount() const
{
    int activeCount = 0;
    for (const ActiveEffect& effect : myActiveEffects)
    {
        if (effect.isActive)
        {
            ++activeCount;
        }
    }

    return activeCount;
}

int VfxSystem::GetCapacity() const
{
    return kMaxActiveEffects;
}

int VfxSystem::GetDefinitionCount() const
{
    return static_cast<int>(myDefinitions.size());
}

std::vector<std::string> VfxSystem::GetEffectIds() const
{
    std::vector<std::string> ids;
    ids.reserve(myDefinitions.size());

    for (const auto& [id, definition] : myDefinitions)
    {
        (void)definition;
        ids.push_back(id);
    }

    std::sort(ids.begin(), ids.end());
    return ids;
}

bool VfxSystem::LoadEffectDefinitions()
{
    namespace fs = std::filesystem;

    myDefinitions.clear();

    const fs::path effectsRoot = Tga::Settings::GameAssetRoot() / "Vfx";
    if (!fs::exists(effectsRoot))
    {
        std::cout << "[VFX] No Vfx folder found at: " << effectsRoot.string() << "\n";
        return true;
    }

    int loadedCount = 0;
    for (const auto& entry : fs::recursive_directory_iterator(effectsRoot))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        const fs::path extension = entry.path().extension();
        if (extension != ".tgvfx" && extension != ".json")
        {
            continue;
        }

        if (LoadDefinitionFromFile(entry.path().string()))
        {
            ++loadedCount;
        }
    }

    std::cout << "[VFX] Loaded " << loadedCount << " effect definitions\n";
    return true;
}

bool VfxSystem::LoadDefinitionFromFile(const std::string& aPath)
{
    std::ifstream input(aPath);
    if (!input.is_open())
    {
        return false;
    }

    Json json;
    try
    {
        input >> json;
    }
    catch (const std::exception&)
    {
        std::cout << "[VFX] Failed to parse " << aPath << "\n";
        return false;
    }

    EffectDefinition definition;

    definition.id = json.value("id", std::filesystem::path(aPath).stem().string());

    definition.texturePath = NormalizeTexturePath(json.value("texture", std::string{}));

    definition.space = ParseSpace(json.value("space", std::string("world")));
    definition.columns = ClampPositive(json.value("columns", 1), 1);
    definition.rows = ClampPositive(json.value("rows", 1), 1);
    definition.frameCount = ClampPositive(json.value("frameCount", definition.columns * definition.rows), definition.columns * definition.rows);
    definition.fps = ClampPositive(json.value("fps", 24.0f), 24.0f);
    definition.loop = json.value("loop", false);
    definition.debugLogFrames = json.value("debugLogFrames", false);
    const float lifetimeSeconds = json.value("lifetime", 0.0f);
    definition.lifetimeSeconds = lifetimeSeconds > 0.0f ? lifetimeSeconds : 0.0f;

    definition.size = ParseVector2(json.value("size", Json::array()), { 100.0f, 100.0f });
    definition.pivot = ParseVector2(json.value("pivot", Json::array()), { 0.5f, 0.5f });
    definition.spawnOffsetWorld = ParseVector3(json.value("spawnOffsetWorld", Json::array()), { 0.0f, 0.0f, 0.0f });
    definition.spawnOffsetScreen = ParseVector2(json.value("spawnOffsetScreen", Json::array()), { 0.0f, 0.0f });
    definition.useOwnerForward = json.value("useOwnerForward", false);
    definition.motionMode = ParseParticleMotionMode(json.value("motion", std::string("static")));
    definition.initialVelocity = ParseVector3(json.value("initialVelocity", Json::array()), { 0.0f, 0.0f, 0.0f });
    definition.acceleration = ParseVector3(json.value("acceleration", Json::array()), { 0.0f, 0.0f, 0.0f });
    definition.driftStrength = json.value("driftStrength", 0.0f);
    definition.driftFrequency = json.value("driftFrequency", 0.0f);
    definition.color = ParseColor(json.value("color", Json::array()), { 1.0f, 1.0f, 1.0f, 1.0f });

    if (definition.id.empty() || definition.texturePath.empty())
    {
        std::cout << "[VFX] Invalid definition: " << aPath << "\n";
        return false;
    }

    myDefinitions[definition.id] = std::move(definition);
    return true;
}

float VfxSystem::CalculateDurationSeconds(const EffectDefinition& aDefinition) const
{
    if (aDefinition.lifetimeSeconds > 0.0f)
    {
        return aDefinition.lifetimeSeconds;
    }

    const int frameCount = ClampPositive(aDefinition.frameCount, 1);
    const float fps = ClampPositive(aDefinition.fps, 1.0f);
    return static_cast<float>(frameCount) / fps;
}

void VfxService::Set(VfxSystem* aSystem)
{
    ourSystem = aSystem;
}

VfxSystem* VfxService::Get()
{
    return ourSystem;
}

bool VfxService::SpawnWorldEffect(const std::string& anEffectId, const CommonUtilities::Vector3<float>& aPosition, const float aSizeMultiplier, const float anOwnerForwardSign)
{
    if (!ourSystem)
    {
        return false;
    }

    return ourSystem->SpawnWorldEffect(anEffectId, aPosition, aSizeMultiplier, anOwnerForwardSign);
}

bool VfxService::SpawnScreenEffect(const std::string& anEffectId, const CommonUtilities::Vector2<float>& aPosition, const float aSizeMultiplier)
{
    if (!ourSystem)
    {
        return false;
    }

    return ourSystem->SpawnScreenEffect(anEffectId, aPosition, aSizeMultiplier);
}
