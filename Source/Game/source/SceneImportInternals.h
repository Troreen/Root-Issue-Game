#pragma once

#include <tge/scene/SceneObjectDefinition.h>
#include <tge/stringRegistry/StringRegistry.h>

#include <any>
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

struct MeshTextureOverrides;

namespace Tga
{
    class SceneObjectDefinitionManager;
}

namespace SceneImportInternal
{
    struct PrefabData
    {
        std::string factoryType;
        std::unordered_map<std::string, std::any> properties;
    };

    double GetElapsedMilliseconds(std::chrono::steady_clock::time_point aStartTime);
    void EnsureScenePropertyTypesRegistered();
    Tga::SceneObjectDefinitionManager& GetCachedSceneObjectDefinitions();
    std::string TrimWhitespace(std::string aValue);
    PrefabData ParsePrefab(const std::string& aPath);
    std::string ResolvePrefabPath(const std::string& aTypeId);
    bool HasAnyModelTextureOverride(const MeshTextureOverrides& someTextureOverrides);
    bool TryGetStringProperty(
        const std::vector<Tga::ScenePropertyDefinition>& aProperties,
        const Tga::StringId& aPropertyName,
        std::string& outValue);
}
