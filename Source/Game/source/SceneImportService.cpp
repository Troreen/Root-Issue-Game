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

namespace
{
    using Json = nlohmann::json;

    struct PrefabData
    {
        std::string factoryType;
        std::unordered_map<std::string, std::any> properties;
    };

    double GetElapsedMilliseconds(const std::chrono::steady_clock::time_point aStartTime)
    {
        return std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - aStartTime).count();
    }

    Tga::SceneObjectDefinitionManager& GetCachedSceneObjectDefinitions()
    {
        static Tga::SceneObjectDefinitionManager manager;
        static std::once_flag initOnce;

        Tga::LoadingProfiler::Scope scope("SceneObjectDefinitionManager::GetCached");
        std::call_once(initOnce, []()
            {
                manager.Init(Tga::Settings::GameAssetRoot().string().c_str());
            });

        return manager;
    }

    std::string TrimWhitespace(std::string aValue)
    {
        const auto notSpace = [](unsigned char aChar) { return std::isspace(aChar) == 0; };
        aValue.erase(aValue.begin(), std::find_if(aValue.begin(), aValue.end(), notSpace));
        aValue.erase(std::find_if(aValue.rbegin(), aValue.rend(), notSpace).base(), aValue.end());
        return aValue;
    }

    std::string ToLower(std::string aValue)
    {
        std::transform(
            aValue.begin(),
            aValue.end(),
            aValue.begin(),
            [](unsigned char aChar) { return static_cast<char>(std::tolower(aChar)); });
        return aValue;
    }

    bool IsComment(const std::string& aLine)
    {
        if (aLine.empty())
        {
            return true;
        }

        if (aLine[0] == '#' || aLine[0] == ';')
        {
            return true;
        }

        return aLine.size() >= 2 && aLine[0] == '/' && aLine[1] == '/';
    }

    bool TryParseBool(const std::string& aValue, bool& outValue)
    {
        const std::string lower = ToLower(aValue);
        if (lower == "true")
        {
            outValue = true;
            return true;
        }

        if (lower == "false")
        {
            outValue = false;
            return true;
        }

        return false;
    }

    bool TryParseInt(const std::string& aValue, int& outValue)
    {
        if (aValue.empty())
        {
            return false;
        }

        const char* begin = aValue.data();
        const char* end = aValue.data() + aValue.size();
        const auto result = std::from_chars(begin, end, outValue);
        return result.ec == std::errc() && result.ptr == end;
    }

    bool TryParseFloat(const std::string& aValue, float& outValue)
    {
        if (aValue.empty())
        {
            return false;
        }

        char* parseEnd = nullptr;
        outValue = std::strtof(aValue.c_str(), &parseEnd);
        return parseEnd == (aValue.c_str() + aValue.size());
    }

    bool TryParseVector3(const std::string& aValue, CommonUtilities::Vector3<float>& outValue)
    {
        if (aValue.size() < 5 || aValue.front() != '(' || aValue.back() != ')')
        {
            return false;
        }

        const std::string inner = aValue.substr(1, aValue.size() - 2);
        const size_t firstComma = inner.find(',');
        if (firstComma == std::string::npos)
        {
            return false;
        }

        const size_t secondComma = inner.find(',', firstComma + 1);
        if (secondComma == std::string::npos)
        {
            return false;
        }

        const std::string xText = TrimWhitespace(inner.substr(0, firstComma));
        const std::string yText = TrimWhitespace(inner.substr(firstComma + 1, secondComma - firstComma - 1));
        const std::string zText = TrimWhitespace(inner.substr(secondComma + 1));

        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        if (!TryParseFloat(xText, x) || !TryParseFloat(yText, y) || !TryParseFloat(zText, z))
        {
            return false;
        }

        outValue = CommonUtilities::Vector3<float>(x, y, z);
        return true;
    }

    std::any ParseLegacyValue(const std::string& aRawValue)
    {
        CommonUtilities::Vector3<float> vec3;
        if (TryParseVector3(aRawValue, vec3))
        {
            return vec3;
        }

        bool boolValue = false;
        if (TryParseBool(aRawValue, boolValue))
        {
            return boolValue;
        }

        if (aRawValue.find('.') == std::string::npos)
        {
            int intValue = 0;
            if (TryParseInt(aRawValue, intValue))
            {
                return intValue;
            }
        }

        float floatValue = 0.0f;
        if (TryParseFloat(aRawValue, floatValue))
        {
            return floatValue;
        }

        return aRawValue;
    }

    std::string ReadFileText(std::ifstream& aFile)
    {
        std::ostringstream stream;
        stream << aFile.rdbuf();
        return stream.str();
    }

    PrefabData ParseLegacyKeyValue(const std::string& aText, const std::string& aPath)
    {
        PrefabData data;

        std::istringstream textStream(aText);
        std::string line;
        int lineNumber = 0;
        while (std::getline(textStream, line))
        {
            ++lineNumber;
            const std::string trimmed = TrimWhitespace(line);
            if (IsComment(trimmed))
            {
                continue;
            }

            const size_t separator = trimmed.find('=');
            if (separator == std::string::npos)
            {
                continue;
            }

            const std::string key = TrimWhitespace(trimmed.substr(0, separator));
            const std::string value = TrimWhitespace(trimmed.substr(separator + 1));
            if (key.empty() || value.empty())
            {
                throw std::runtime_error("ParsePrefab malformed entry at line " + std::to_string(lineNumber));
            }

            if (key == "factoryType")
            {
                data.factoryType = value;
                continue;
            }

            data.properties[key] = ParseLegacyValue(value);
        }

        if (data.factoryType.empty())
        {
            throw std::runtime_error("ParsePrefab missing required field 'factoryType' in file: " + aPath);
        }

        return data;
    }

    std::any ParseJsonPropertyValue(const std::string& aType, const Json& aValue)
    {
        if (aValue.is_null())
        {
            return std::any();
        }

        if (aType == "StringId" || aType == "String")
        {
            return aValue.is_string() ? std::any(aValue.get<std::string>()) : std::any(std::string());
        }

        if (aType == "Bool")
        {
            return aValue.is_boolean() ? std::any(aValue.get<bool>()) : std::any(false);
        }

        if (aType == "Int")
        {
            return aValue.is_number_integer() ? std::any(aValue.get<int>()) : std::any(0);
        }

        if (aType == "Float")
        {
            return aValue.is_number() ? std::any(aValue.get<float>()) : std::any(0.0f);
        }

        if (aType == "Float3")
        {
            if (aValue.is_array() && aValue.size() >= 3 &&
                aValue[0].is_number() && aValue[1].is_number() && aValue[2].is_number())
            {
                return std::any(CommonUtilities::Vector3<float>(
                    aValue[0].get<float>(),
                    aValue[1].get<float>(),
                    aValue[2].get<float>()));
            }

            return std::any(CommonUtilities::Vector3<float>(0.0f, 0.0f, 0.0f));
        }

        if (aValue.is_number_integer())
        {
            return std::any(aValue.get<int>());
        }

        if (aValue.is_number_float())
        {
            return std::any(aValue.get<float>());
        }

        if (aValue.is_boolean())
        {
            return std::any(aValue.get<bool>());
        }

        if (aValue.is_string())
        {
            return std::any(aValue.get<std::string>());
        }

        return std::any();
    }

    bool TryParseModelTextureOverrides(const Json& someTextures, MeshTextureOverrides& outTextureOverrides)
    {
        if (!someTextures.is_array())
        {
            return false;
        }

        bool hasAnyOverrides = false;
        int meshIndex = 0;
        for (const Json& perMeshTextures : someTextures)
        {
            if (meshIndex >= MeshTextureOverrides::kMaxMeshCount)
            {
                break;
            }

            if (!perMeshTextures.is_array())
            {
                ++meshIndex;
                continue;
            }

            int textureIndex = 0;
            for (const Json& texturePath : perMeshTextures)
            {
                if (textureIndex >= MeshTextureOverrides::kTextureChannelCount)
                {
                    break;
                }

                if (texturePath.is_string())
                {
                    const std::string texturePathString = texturePath.get<std::string>();
                    if (!texturePathString.empty())
                    {
                        outTextureOverrides.textures[meshIndex][textureIndex] = texturePathString;
                        hasAnyOverrides = true;
                    }
                }

                ++textureIndex;
            }

            ++meshIndex;
        }

        return hasAnyOverrides;
    }

    bool HasAnyModelTextureOverride(const MeshTextureOverrides& someTextureOverrides)
    {
        for (int meshIndex = 0; meshIndex < MeshTextureOverrides::kMaxMeshCount; ++meshIndex)
        {
            for (int textureIndex = 0; textureIndex < MeshTextureOverrides::kTextureChannelCount; ++textureIndex)
            {
                if (!someTextureOverrides.textures[meshIndex][textureIndex].empty())
                {
                    return true;
                }
            }
        }

        return false;
    }

    PrefabData ParseJsonTgo(const std::string& aText, const std::string& aPath)
    {
        PrefabData data;

        Json root;
        try
        {
            root = Json::parse(aText);
        }
        catch (const Json::exception& anError)
        {
            throw std::runtime_error(
                "ParsePrefab failed to parse JSON in file: " + aPath + " (" + anError.what() + ")");
        }

        if (!root.is_object())
        {
            throw std::runtime_error("ParsePrefab invalid JSON object in file: " + aPath);
        }

        const Json properties = root.value("properties", Json::array());
        if (!properties.is_array())
        {
            throw std::runtime_error("ParsePrefab missing properties array in file: " + aPath);
        }

        for (const Json& property : properties)
        {
            if (!property.is_object() || !property.contains("name") || !property.contains("value"))
            {
                continue;
            }

            const std::string name = property.value("name", "");
            const std::string type = property.value("type", "");
            const Json value = property["value"];
            if (name.empty())
            {
                continue;
            }

            if (name == "factoryType")
            {
                if (value.is_string())
                {
                    data.factoryType = value.get<std::string>();
                }

                continue;
            }

            if (type == "Model")
            {
                if (value.is_object() && value.contains("path") && value["path"].is_string())
                {
                    data.properties["modelPath"] = value["path"].get<std::string>();
                    data.properties["model"] = value["path"].get<std::string>();
                }

                MeshTextureOverrides textureOverrides;
                if (value.is_object() && value.contains("textures") &&
                    TryParseModelTextureOverrides(value["textures"], textureOverrides))
                {
                    data.properties["modelTextures"] = textureOverrides;
                }

                continue;
            }

            if (type == "Animation Clip")
            {
                if (value.is_object() && value.contains("clip_path") && value["clip_path"].is_string())
                {
                    data.properties[name] = value["clip_path"].get<std::string>();
                }

                continue;
            }

            if (type == "Scene")
            {
                if (value.is_object() && value.contains("path") && value["path"].is_string())
                {
                    data.properties[name] = value["path"].get<std::string>();
                }

                continue;
            }

            const std::any parsedValue = ParseJsonPropertyValue(type, value);
            if (parsedValue.has_value())
            {
                data.properties[name] = parsedValue;
            }
        }

        if (data.factoryType.empty())
        {
            throw std::runtime_error("ParsePrefab missing required field 'factoryType' in file: " + aPath);
        }

        return data;
    }

    PrefabData ParsePrefab(const std::string& aPath)
    {
        std::ifstream prefabFile(aPath);
        if (!prefabFile.is_open())
        {
            const std::string resolvedPath = Tga::Settings::ResolveAssetPath(aPath);
            if (!resolvedPath.empty())
            {
                prefabFile.open(resolvedPath);
            }

            if (!prefabFile.is_open())
            {
                const std::filesystem::path rootedPath = Tga::Settings::GameAssetRoot() / aPath;
                prefabFile.open(rootedPath);
            }
        }

        if (!prefabFile.is_open())
        {
            throw std::runtime_error("ParsePrefab failed to open file: " + aPath);
        }

        const std::string fileText = ReadFileText(prefabFile);
        const std::string trimmed = TrimWhitespace(fileText);
        if (!trimmed.empty() && trimmed.front() == '{')
        {
            return ParseJsonTgo(fileText, aPath);
        }

        return ParseLegacyKeyValue(fileText, aPath);
    }

    std::string ResolvePrefabPath(const std::string& aTypeId)
    {
        std::string normalizedTypeId = TrimWhitespace(aTypeId);
        if (normalizedTypeId.empty())
        {
            return {};
        }

        std::string directPath = "Objects/" + normalizedTypeId;
        if (std::filesystem::path(directPath).extension() != ".tgo")
        {
            directPath += ".tgo";
        }

        if (!Tga::Settings::ResolveAssetPath(directPath).empty())
        {
            return directPath;
        }

        const std::filesystem::path gameAssetRoot = Tga::Settings::GameAssetRoot();
        const std::filesystem::path objectsRoot = gameAssetRoot / "Objects";
        const std::filesystem::path directRootedPath = gameAssetRoot / directPath;
        if (std::filesystem::exists(directRootedPath))
        {
            return directPath;
        }

        if (!std::filesystem::exists(objectsRoot) || !std::filesystem::is_directory(objectsRoot))
        {
            return directPath;
        }

        const std::string targetFileName = ToLower(std::filesystem::path(directPath).filename().string());

        try
        {
            for (const std::filesystem::directory_entry& entry :
                 std::filesystem::recursive_directory_iterator(objectsRoot))
            {
                if (!entry.is_regular_file())
                {
                    continue;
                }

                const std::filesystem::path candidatePath = entry.path();
                if (candidatePath.extension() != ".tgo")
                {
                    continue;
                }

                const std::string candidateFileName = ToLower(candidatePath.filename().string());
                if (candidateFileName != targetFileName)
                {
                    continue;
                }

                const std::filesystem::path relativePath = std::filesystem::relative(candidatePath, gameAssetRoot);
                return relativePath.generic_string();
            }
        }
        catch (const std::exception&)
        {
            // Fall back to direct path if recursive lookup fails unexpectedly.
        }

        return directPath;
    }

    /// Extract a StringId property and convert to std::string.
    bool TryGetStringProperty(
        const std::vector<Tga::ScenePropertyDefinition>& aProperties,
        const Tga::StringId& aPropertyName,
        std::string& outValue)
    {
        for (const auto& prop : aProperties)
        {
            if (prop.name == aPropertyName)
            {
                if (auto* strIdPtr = prop.value.Get<Tga::StringId>())
                {
                    outValue = strIdPtr->GetString();
                    return true;
                }
            }
        }
        return false;
    }

}

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

    std::vector<std::unique_ptr<GameObject>> objects;
    objects.reserve(someSceneObjects.size());
    double prefabParseMilliseconds = 0.0;
    double propertyMergeMilliseconds = 0.0;
    double factoryCreateMilliseconds = 0.0;

    for (const SceneObjectData& sceneObject : someSceneObjects)
    {
        if (sceneObject.typeId.empty())
        {
            continue;
        }

        const std::string prefabPath = ResolvePrefabPath(sceneObject.typeId);

        try
        {
            auto phaseStartTime = std::chrono::steady_clock::now();
            const PrefabData prefabData = ParsePrefab(prefabPath);
            prefabParseMilliseconds += GetElapsedMilliseconds(phaseStartTime);

            phaseStartTime = std::chrono::steady_clock::now();
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

    Tga::LoadingProfiler::GetInstance().RecordPhase("SceneImportService::PrefabParse", prefabParseMilliseconds);
    Tga::LoadingProfiler::GetInstance().RecordPhase("SceneImportService::PrefabMerge", propertyMergeMilliseconds);
    Tga::LoadingProfiler::GetInstance().RecordPhase("SceneImportService::FactoryCreate", factoryCreateMilliseconds);

    return objects;
}
