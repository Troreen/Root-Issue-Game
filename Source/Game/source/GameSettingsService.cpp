#include "GameSettingsService.h"

#include <tge/settings/Settings.h>

#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

namespace
{
    using Json = nlohmann::json;

    constexpr const char* kGameSettingsFileName = "GameSettings.json";
    constexpr const char* kSceneLightingKey = "sceneLighting";

    std::string locLastStatusMessage;

    std::filesystem::path GetSettingsPath()
    {
        return Tga::Settings::GameAssetRoot() / kGameSettingsFileName;
    }

    std::string AmbientModeToString(const Tga::AmbientLightType anAmbientMode)
    {
        switch (anAmbientMode)
        {
        case Tga::AmbientLightType::UniformAboveHorizon:
            return "UniformAboveHorizon";
        case Tga::AmbientLightType::Custom:
            return "Custom";
        case Tga::AmbientLightType::Uniform:
        default:
            return "Uniform";
        }
    }

    Tga::AmbientLightType AmbientModeFromString(const std::string& anAmbientMode)
    {
        if (anAmbientMode == "UniformAboveHorizon")
        {
            return Tga::AmbientLightType::UniformAboveHorizon;
        }
        if (anAmbientMode == "Custom")
        {
            return Tga::AmbientLightType::Custom;
        }

        return Tga::AmbientLightType::Uniform;
    }

    Json LoadSettingsJson()
    {
        const std::filesystem::path settingsPath = GetSettingsPath();
        if (!std::filesystem::exists(settingsPath))
        {
            return Json::object();
        }

        std::ifstream input(settingsPath);
        if (!input)
        {
            locLastStatusMessage = "Failed to open GameSettings.json for reading.";
            return Json::object();
        }

        Json json = Json::object();
        try
        {
            input >> json;
        }
        catch (const std::exception& e)
        {
            locLastStatusMessage = std::string("Failed to parse GameSettings.json: ") + e.what();
            return Json::object();
        }

        if (!json.is_object())
        {
            return Json::object();
        }

        return json;
    }

    bool WriteSettingsJson(const Json& aJson)
    {
        const std::filesystem::path settingsPath = GetSettingsPath();
        std::error_code errorCode;
        std::filesystem::create_directories(settingsPath.parent_path(), errorCode);
        if (errorCode)
        {
            locLastStatusMessage = "Failed to create data folder for GameSettings.json.";
            return false;
        }

        std::ofstream output(settingsPath);
        if (!output)
        {
            locLastStatusMessage = "Failed to open GameSettings.json for writing.";
            return false;
        }

        output << aJson.dump(2);
        locLastStatusMessage = "Saved lighting to " + settingsPath.generic_string();
        return true;
    }

    Json ToJson(const GameDebugSettings::LightingSettings& someSettings)
    {
        return Json{
            { "enableDirectionalLight", someSettings.enableDirectionalLight },
            { "directionalRotationDegrees", Json::array({
                someSettings.directionalRotationDegrees[0],
                someSettings.directionalRotationDegrees[1],
                someSettings.directionalRotationDegrees[2] }) },
            { "directionalColor", Json::array({
                someSettings.directionalColor[0],
                someSettings.directionalColor[1],
                someSettings.directionalColor[2] }) },
            { "directionalIntensity", someSettings.directionalIntensity },
            { "directionalSoftness", someSettings.directionalSoftness },
            { "enableAmbientLight", someSettings.enableAmbientLight },
            { "ambientMode", AmbientModeToString(someSettings.ambientMode) },
            { "ambientColor", Json::array({
                someSettings.ambientColor[0],
                someSettings.ambientColor[1],
                someSettings.ambientColor[2] }) },
            { "ambientIntensity", someSettings.ambientIntensity }
        };
    }

    void ReadFloatArray3(const Json& aJson, const char* aKey, float* outValues)
    {
        if (!aJson.contains(aKey) || !aJson[aKey].is_array() || aJson[aKey].size() < 3)
        {
            return;
        }

        for (int index = 0; index < 3; ++index)
        {
            if (aJson[aKey][index].is_number())
            {
                outValues[index] = aJson[aKey][index].get<float>();
            }
        }
    }

    GameDebugSettings::LightingSettings FromJson(const Json& aJson)
    {
        GameDebugSettings::LightingSettings settings = GameDebugSettings::MakeDefaultLightingSettings();
        settings.enableDirectionalLight = aJson.value("enableDirectionalLight", settings.enableDirectionalLight);
        ReadFloatArray3(aJson, "directionalRotationDegrees", settings.directionalRotationDegrees);
        ReadFloatArray3(aJson, "directionalColor", settings.directionalColor);
        if (aJson.contains("directionalSoftness"))
        {
            settings.directionalIntensity = aJson.value("directionalIntensity", settings.directionalIntensity);
            settings.directionalSoftness = aJson.value("directionalSoftness", settings.directionalSoftness);
        }
        else
        {
            settings.directionalSoftness = aJson.value("directionalIntensity", settings.directionalSoftness);
        }
        settings.enableAmbientLight = aJson.value("enableAmbientLight", settings.enableAmbientLight);
        settings.ambientMode = AmbientModeFromString(aJson.value("ambientMode", AmbientModeToString(settings.ambientMode)));
        ReadFloatArray3(aJson, "ambientColor", settings.ambientColor);
        settings.ambientIntensity = aJson.value("ambientIntensity", settings.ambientIntensity);
        return settings;
    }
}

bool GameSettingsService::TryApplySceneLighting(const std::string& aScenePath)
{
    GameDebugSettings::ResetLightingSettingsToDefaults();

    const std::string scenePath = NormalizeScenePath(aScenePath);
    if (scenePath.empty())
    {
        locLastStatusMessage = "No scene path provided for lighting settings.";
        return false;
    }

    const Json settingsJson = LoadSettingsJson();
    if (!settingsJson.contains(kSceneLightingKey) || !settingsJson[kSceneLightingKey].is_object())
    {
        locLastStatusMessage = "No scene lighting settings found; using defaults.";
        return false;
    }

    const Json& sceneLighting = settingsJson[kSceneLightingKey];
    if (!sceneLighting.contains(scenePath) || !sceneLighting[scenePath].is_object())
    {
        locLastStatusMessage = "No lighting override for " + scenePath + "; using defaults.";
        return false;
    }

    GameDebugSettings::SetLightingSettings(FromJson(sceneLighting[scenePath]));
    locLastStatusMessage = "Applied lighting for " + scenePath + ".";
    return true;
}

bool GameSettingsService::SaveSceneLighting(
    const std::string& aScenePath,
    const GameDebugSettings::LightingSettings& someSettings)
{
    const std::string scenePath = NormalizeScenePath(aScenePath);
    if (scenePath.empty())
    {
        locLastStatusMessage = "Select a scene before saving lighting.";
        return false;
    }

    Json settingsJson = LoadSettingsJson();
    if (!settingsJson.contains(kSceneLightingKey) || !settingsJson[kSceneLightingKey].is_object())
    {
        settingsJson[kSceneLightingKey] = Json::object();
    }

    settingsJson[kSceneLightingKey][scenePath] = ToJson(someSettings);
    return WriteSettingsJson(settingsJson);
}

std::string GameSettingsService::NormalizeScenePath(const std::string& aScenePath)
{
    if (aScenePath.empty())
    {
        return {};
    }

    std::filesystem::path scenePath(aScenePath);
    if (scenePath.is_absolute())
    {
        std::error_code errorCode;
        const std::filesystem::path relativePath = std::filesystem::relative(
            scenePath,
            Tga::Settings::GameAssetRoot(),
            errorCode);
        if (!errorCode)
        {
            scenePath = relativePath;
        }
    }

    std::string normalized = scenePath.lexically_normal().generic_string();
    constexpr const char* dataPrefix = "data/";
    if (normalized.rfind(dataPrefix, 0) == 0)
    {
        normalized.erase(0, std::char_traits<char>::length(dataPrefix));
    }

    return normalized;
}

const std::string& GameSettingsService::GetLastStatusMessage()
{
    return locLastStatusMessage;
}
