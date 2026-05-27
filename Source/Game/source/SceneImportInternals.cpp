#include "SceneImportInternals.h"

#include "ModelTextureOverrides.h"
#include "SceneObjectData.h"

#include <CommonUtilities/Quaternion.hpp>
#include <tge/Model/ModelFactory.h>
#include <tge/Math/Quaternion.h>
#include <tge/debug/LoadingProfiler.h>
#include <tge/error/ErrorManager.h>
#include <tge/scene/SceneObjectDefinitionManager.h>
#include <tge/script/BaseProperties.h>
#include <tge/settings/Settings.h>
#include <tge/math/Color.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
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
#include <utility>

namespace SceneImportInternal
{
    using Json = nlohmann::json;

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

    bool TryParseSceneSpriteData(const Json& aValue, SceneSpriteData& outSpriteData)
    {
        if (!aValue.is_object())
        {
            return false;
        }

        std::array<std::string, SceneSpriteData::kTextureSlotCount> texturePaths = {};
        if (aValue.contains("texturePath") && aValue["texturePath"].is_string())
        {
            texturePaths[0] = aValue["texturePath"].get<std::string>();
        }
        else if (aValue.contains("textures") && aValue["textures"].is_array())
        {
            const Json& textures = aValue["textures"];
            const std::size_t textureCount = (std::min<std::size_t>)(textures.size(), texturePaths.size());
            for (std::size_t textureIndex = 0; textureIndex < textureCount; ++textureIndex)
            {
                if (textures[textureIndex].is_string())
                {
                    texturePaths[textureIndex] = textures[textureIndex].get<std::string>();
                }
            }
        }

        if (texturePaths[0].empty())
        {
            return false;
        }

        SceneSpriteData spriteData;
        spriteData.texturePaths = std::move(texturePaths);
        spriteData.texturePath = spriteData.texturePaths[0];

        if (aValue.contains("size") && aValue["size"].is_array() && aValue["size"].size() >= 2 &&
            aValue["size"][0].is_number() && aValue["size"][1].is_number())
        {
            spriteData.size = {
                aValue["size"][0].get<float>(),
                aValue["size"][1].get<float>()
            };
        }

        if (aValue.contains("pivot") && aValue["pivot"].is_array() && aValue["pivot"].size() >= 2 &&
            aValue["pivot"][0].is_number() && aValue["pivot"][1].is_number())
        {
            spriteData.pivot = {
                aValue["pivot"][0].get<float>(),
                aValue["pivot"][1].get<float>()
            };
        }

        outSpriteData = std::move(spriteData);
        return true;
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

        bool hasSpriteProperty = false;
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

            if (type == "Sprite")
            {
                SceneSpriteData spriteData;
                if (TryParseSceneSpriteData(value, spriteData))
                {
                    data.properties[name] = std::move(spriteData);
                    hasSpriteProperty = true;
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

        if (data.factoryType.empty() && hasSpriteProperty)
        {
            data.factoryType = "StaticWorld";
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
