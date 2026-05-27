#include "SceneImportService.h"

#include "GameObject.h"
#include "GameObjectFactory.h"
#include "ModelTextureOverrides.h"
#include "SceneObjectData.h"

#include <CommonUtilities/Quaternion.hpp>
#include <tge/debug/LoadingProfiler.h>
#include <tge/Model/ModelFactory.h>
#include <tge/Math/Quaternion.h>
#include <tge/error/ErrorManager.h>
#include <tge/scene/Scene.h>
#include <tge/scene/SceneSerialize.h>
#include <tge/scene/SceneObjectDefinitionManager.h>
#include <tge/scene/ScenePropertyTypes.h>
#include <tge/script/BaseProperties.h>
#include <tge/settings/Settings.h>
#include <tge/math/Color.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "SceneImportInternals.h"

using namespace SceneImportInternal;

std::vector<SceneObjectData> SceneImportService::LoadSceneObjects(const std::string& scenePath) const
{
    Tga::LoadingProfiler::Scope loadScope("SceneImportService::LoadSceneObjects");
    std::vector<SceneObjectData> sceneObjects;

    Tga::Scene scene;
    if (!Tga::LoadScene(scenePath.c_str(), scene))
    {
        std::cout << "[SceneImport] Failed to load scene file: " << scenePath << "\n";
        return sceneObjects;
    }
    Tga::LoadingProfiler::GetInstance().RecordObjectCount(scene.GetSceneObjects().size());

    Tga::SceneObjectDefinitionManager& sceneDefinitionManager = GetCachedSceneObjectDefinitions();

    std::vector<Tga::ScenePropertyDefinition> sceneObjectProperties;
    sceneObjectProperties.reserve(32);
    sceneObjects.reserve(scene.GetSceneObjects().size());

    double propertyMergeMilliseconds = 0.0;
    double dataExtractionMilliseconds = 0.0;
    double objectLoopMilliseconds = 0.0;
    
    for (const auto& p : scene.GetSceneObjects())
    {
        const auto objectLoopStartTime = std::chrono::steady_clock::now();
        sceneObjectProperties.clear();
        auto phaseStartTime = std::chrono::steady_clock::now();
        p.second->CalculateCombinedPropertySet(sceneDefinitionManager, sceneObjectProperties);
        propertyMergeMilliseconds += GetElapsedMilliseconds(phaseStartTime);

        // Build SceneObjectData from editor properties
        phaseStartTime = std::chrono::steady_clock::now();
        SceneObjectData data;
        data.name = p.second->GetName();

        data.typeId = TrimWhitespace(std::string(p.second->GetSceneObjectDefinitionName().GetString()));
        

        // Get the optional "renderMode" property (Default, Lambert, Pbr)
        std::string renderMode;
        if (TryGetStringProperty(sceneObjectProperties, "renderMode"_tgaid, renderMode))
        {
            data.properties["renderMode"] = renderMode;
        }

        // Extract model path if present
        for (const Tga::ScenePropertyDefinition& property : sceneObjectProperties)
        {
            if (property.type == Tga::GetPropertyType<Tga::CopyOnWriteWrapper<Tga::SceneModel>>())
            {
                const Tga::SceneModel& value = property.value.Get<Tga::CopyOnWriteWrapper<Tga::SceneModel>>()->Get();

                Tga::StringId path = value.path;
                if (path.IsEmpty() || Tga::Settings::ResolveAssetPath(path.GetString()).empty())
                {
                    continue;
                }

                // Store model path for both static and animated models
                data.properties["modelPath"] = std::string(path.GetString());
                data.properties["model"] = std::string(path.GetString());
                Tga::LoadingProfiler::GetInstance().RecordModelPath(path.GetString());

                // Extract texture overrides regardless of model type (static or animated)
                MeshTextureOverrides textureOverrides;
                bool hasAnyTextureOverrides = false;
                for (int meshIndex = 0; meshIndex < MeshTextureOverrides::kMaxMeshCount; ++meshIndex)
                {
                    for (int textureIndex = 0; textureIndex < MeshTextureOverrides::kTextureChannelCount; ++textureIndex)
                    {
                        const Tga::StringId textureId = value.textures[meshIndex][textureIndex];
                        if (!textureId.IsEmpty())
                        {
                            textureOverrides.textures[meshIndex][textureIndex] = textureId.GetString();
                            hasAnyTextureOverrides = true;
                            Tga::LoadingProfiler::GetInstance().RecordTexturePath(textureId.GetString());
                        }
                    }
                }

                if (hasAnyTextureOverrides)
                {
                    data.properties["modelTextures"] = textureOverrides;
                }
            }
            else if (property.type == Tga::GetPropertyType<Tga::CopyOnWriteWrapper<Tga::SceneSprite>>())
            {
                const Tga::SceneSprite& value = property.value.Get<Tga::CopyOnWriteWrapper<Tga::SceneSprite>>()->Get();
                if (!value.textures[0].IsEmpty())
                {
                    SceneSpriteData spriteData;
                    for (int textureIndex = 0; textureIndex < SceneSpriteData::kTextureSlotCount; ++textureIndex)
                    {
                        if (!value.textures[textureIndex].IsEmpty())
                        {
                            spriteData.texturePaths[textureIndex] = value.textures[textureIndex].GetString();
                            Tga::LoadingProfiler::GetInstance().RecordTexturePath(value.textures[textureIndex].GetString());
                        }
                    }
                    spriteData.texturePath = spriteData.texturePaths[0];
                    spriteData.size = { value.size.x, value.size.y };
                    spriteData.pivot = { value.pivot.x, value.pivot.y };
                    data.properties[property.name.GetString()] = std::move(spriteData);
                }
            }
        }

        // Extract transform
        const Tga::Matrix4x4f tgaTransform = p.second->GetTransform();
        Tga::Vector3f position;
        Tga::Vector3f scale;
        Tga::Quaternionf rotation;
        tgaTransform.DecomposeMatrix(position, rotation, scale);

        data.position = CommonUtilities::Vector3<float>(position.X, position.Y, position.Z);
        data.rotation = CommonUtilities::Quaternion<float>(rotation.W, rotation.X, rotation.Y, rotation.Z);
        data.scale = CommonUtilities::Vector3<float>(scale.X, scale.Y, scale.Z);

        //Extract Object Definition Name
        const std::string ObjectDef = p.second->GetSceneObjectDefinitionName().GetString();
        data.ObjDefinition = ObjectDef;


        // Extract all custom properties into the properties map
        for (const Tga::ScenePropertyDefinition& property : sceneObjectProperties)
        {
            std::string propName = property.name.GetString();

            // Store typed properties
            if (auto* floatPtr = property.value.Get<float>())
            {
                data.properties[propName] = *floatPtr;
            }
            else if (auto* intPtr = property.value.Get<int>())
            {
                data.properties[propName] = *intPtr;
            }
            else if (auto* boolPtr = property.value.Get<bool>())
            {
                data.properties[propName] = *boolPtr;
            }
            else if (auto* strIdPtr = property.value.Get<Tga::StringId>())
            {
                // Convert StringId to std::string
                data.properties[propName] = std::string(strIdPtr->GetString());
                if (propName == "Tag" || propName == "tag")
                {
                    Tga::LoadingProfiler::GetInstance().RecordTag(strIdPtr->GetString());
                }
            }
            else if (auto* clipRefPtr =
                         property.value.Get<Tga::CopyOnWriteWrapper<Tga::AnimationClipReference>>())
            {
                const std::string clipPath = clipRefPtr->Get().path.GetString();
                data.properties[propName] = clipPath;
                Tga::LoadingProfiler::GetInstance().RecordAnimationPath(clipPath);
            }
            else if (auto* sceneRefPtr =
                         property.value.Get<Tga::CopyOnWriteWrapper<Tga::SceneReference>>())
            {
                data.properties[propName] = std::string(sceneRefPtr->Get().path.GetString());
            }
            else if (auto* spritePtr =
                         property.value.Get<Tga::CopyOnWriteWrapper<Tga::SceneSprite>>())
            {
                const Tga::SceneSprite& sprite = spritePtr->Get();
                if (!sprite.textures[0].IsEmpty())
                {
                    SceneSpriteData spriteData;
                    for (int textureIndex = 0; textureIndex < SceneSpriteData::kTextureSlotCount; ++textureIndex)
                    {
                        if (!sprite.textures[textureIndex].IsEmpty())
                        {
                            spriteData.texturePaths[textureIndex] = sprite.textures[textureIndex].GetString();
                        }
                    }
                    spriteData.texturePath = spriteData.texturePaths[0];
                    spriteData.size = { sprite.size.x, sprite.size.y };
                    spriteData.pivot = { sprite.pivot.x, sprite.pivot.y };
                    data.properties[propName] = std::move(spriteData);
                }
            }
            else if (auto* vec3Ptr = property.value.Get<Tga::Vector3f>())
            {
                data.properties[propName] = CommonUtilities::Vector3<float>(vec3Ptr->X, vec3Ptr->Y, vec3Ptr->Z);
            }
            else if (auto* colorPtr = property.value.Get<Tga::Color>())
            {
                data.properties[propName] = *colorPtr;
            }
        }
        dataExtractionMilliseconds += GetElapsedMilliseconds(phaseStartTime);

        sceneObjects.push_back(std::move(data));
        objectLoopMilliseconds += GetElapsedMilliseconds(objectLoopStartTime);
    }

    Tga::LoadingProfiler::GetInstance().RecordPhase("SceneImportService::PropertyMerge", propertyMergeMilliseconds);
    Tga::LoadingProfiler::GetInstance().RecordPhase("SceneImportService::DataExtraction", dataExtractionMilliseconds);
    Tga::LoadingProfiler::GetInstance().RecordPhase("SceneImportService::ObjectLoopTotal", objectLoopMilliseconds);
    Tga::LoadingProfiler::GetInstance().RecordPhase(
        "SceneImportService::ObjectLoopUntracked",
        objectLoopMilliseconds - propertyMergeMilliseconds - dataExtractionMilliseconds);

    return sceneObjects;
}

std::vector<std::unique_ptr<GameObject>> SceneImportService::BuildGameObjects(
    const std::string& scenePath) const
{
    const std::vector<SceneObjectData> sceneObjects = LoadSceneObjects(scenePath);
    return BuildGameObjects(sceneObjects);
}

std::vector<std::unique_ptr<GameObject>> SceneImportService::BuildGameObjects(
    const std::vector<SceneObjectData>& someSceneObjects) const
{
    Tga::LoadingProfiler::Scope buildScope("SceneImportService::BuildGameObjects");

    auto isAnimationAssetPathProperty = [](const std::string& aPropertyName) {
        return aPropertyName == "animation_graph" || aPropertyName.rfind("clip_", 0) == 0;
    };

    auto tryGetStringAny = [](const std::any& aValue, std::string& outValue) {
        if (const std::string* valuePtr = std::any_cast<std::string>(&aValue))
        {
            outValue = *valuePtr;
            return true;
        }

        return false;
    };

    Tga::LoadingProfiler::GetInstance().RecordObjectCount(someSceneObjects.size());

    std::vector<std::unique_ptr<GameObject>> objects;
    objects.reserve(someSceneObjects.size());
    double prefabResolveMilliseconds = 0.0;
    double prefabParseMilliseconds = 0.0;
    double propertyMergeMilliseconds = 0.0;
    double factoryCreateMilliseconds = 0.0;

    // Level scenes often contain hundreds of instances of the same few prefab types.
    // Cache path resolution and parsed .tgo data for this build so repeated rocks,
    // trees, and tiles do not rescan/reparse the same files on the main thread.
    std::unordered_map<std::string, std::string> prefabPathCache;
    std::unordered_map<std::string, PrefabData> prefabDataCache;

    auto resolvePrefabPathCached = [&](const std::string& aTypeId) -> const std::string& {
        auto cachedPathIt = prefabPathCache.find(aTypeId);
        if (cachedPathIt != prefabPathCache.end())
        {
            return cachedPathIt->second;
        }

        const auto phaseStartTime = std::chrono::steady_clock::now();
        std::string prefabPath = ResolvePrefabPath(aTypeId);
        prefabResolveMilliseconds += GetElapsedMilliseconds(phaseStartTime);

        auto [insertedIt, wasInserted] = prefabPathCache.emplace(aTypeId, std::move(prefabPath));
        UNREFERENCED_PARAMETER(wasInserted);
        return insertedIt->second;
    };

    auto getPrefabDataCached = [&](const std::string& aPrefabPath) -> const PrefabData& {
        auto cachedPrefabIt = prefabDataCache.find(aPrefabPath);
        if (cachedPrefabIt != prefabDataCache.end())
        {
            return cachedPrefabIt->second;
        }

        const auto phaseStartTime = std::chrono::steady_clock::now();
        PrefabData prefabData = ParsePrefab(aPrefabPath);
        prefabParseMilliseconds += GetElapsedMilliseconds(phaseStartTime);

        auto [insertedIt, wasInserted] = prefabDataCache.emplace(aPrefabPath, std::move(prefabData));
        UNREFERENCED_PARAMETER(wasInserted);
        return insertedIt->second;
    };

    for (const SceneObjectData& sceneObject : someSceneObjects)
    {
        if (sceneObject.typeId.empty())
        {
            continue;
        }

        try
        {
            const std::string& prefabPath = resolvePrefabPathCached(sceneObject.typeId);
            const PrefabData& prefabData = getPrefabDataCached(prefabPath);

            auto phaseStartTime = std::chrono::steady_clock::now();
            SceneObjectData merged = sceneObject;
            for (const auto& [name, value] : prefabData.properties)
            {
                auto existingPropertyIt = merged.properties.find(name);
                if (existingPropertyIt == merged.properties.end())
                {
                    merged.properties[name] = value;
                    continue;
                }

                if (name == "modelTextures")
                {
                    // Scene model overrides win when authored per instance. If the instance
                    // only overrides the model path, keep the prefab's texture assignments.
                    const MeshTextureOverrides* mergedTextureOverrides =
                        std::any_cast<MeshTextureOverrides>(&existingPropertyIt->second);
                    const bool hasMergedTextures =
                        mergedTextureOverrides && HasAnyModelTextureOverride(*mergedTextureOverrides);
                    if (!hasMergedTextures)
                    {
                        existingPropertyIt->second = value;
                    }

                    continue;
                }

                if (!isAnimationAssetPathProperty(name))
                {
                    continue;
                }

                std::string mergedPath;
                const bool hasMergedPath = tryGetStringAny(existingPropertyIt->second, mergedPath);
                const bool mergedPathIsValid =
                    hasMergedPath && !mergedPath.empty() && !Tga::Settings::ResolveAssetPath(mergedPath).empty();

                if (mergedPathIsValid)
                {
                    continue;
                }

                std::string prefabAssetPath;
                const bool hasPrefabPath = tryGetStringAny(value, prefabAssetPath);
                const bool prefabPathIsValid =
                    hasPrefabPath && !prefabAssetPath.empty() &&
                    !Tga::Settings::ResolveAssetPath(prefabAssetPath).empty();

                if (prefabPathIsValid)
                {
                    existingPropertyIt->second = value;
                }
            }
            propertyMergeMilliseconds += GetElapsedMilliseconds(phaseStartTime);

            phaseStartTime = std::chrono::steady_clock::now();
            std::unique_ptr<GameObject> gameObject =
                GameObjectFactory::GetInstance().Build(prefabData.factoryType, merged);
            factoryCreateMilliseconds += GetElapsedMilliseconds(phaseStartTime);
            if (gameObject)
            {
                auto& transform = gameObject->GetTransform();
                transform.SetPosition(merged.position);
                transform.SetRotation(merged.rotation);
                transform.SetScale(merged.scale);

                gameObject.get()->SetObjDefinition(sceneObject.ObjDefinition.c_str());

                objects.push_back(std::move(gameObject));
            }
        }
        catch (const std::exception& exception)
        {
            UNREFERENCED_PARAMETER(exception);
            ERROR_PRINT(
                "Failed to build '%s' (typeId: %s): %s",
                sceneObject.name.c_str(),
                sceneObject.typeId.c_str(),
                exception.what());
        }
    }

    Tga::LoadingProfiler::GetInstance().RecordPhase("SceneImportService::PrefabResolvePath", prefabResolveMilliseconds);
    Tga::LoadingProfiler::GetInstance().RecordPhase("SceneImportService::PrefabParse", prefabParseMilliseconds);
    Tga::LoadingProfiler::GetInstance().RecordPhase("SceneImportService::PrefabMerge", propertyMergeMilliseconds);
    Tga::LoadingProfiler::GetInstance().RecordPhase("SceneImportService::FactoryCreate", factoryCreateMilliseconds);

    return objects;
}
