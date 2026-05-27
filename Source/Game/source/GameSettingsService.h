#pragma once

#include "DebugSettings.h"

#include <string>

class GameSettingsService
{
public:
    static bool TryApplySceneLighting(const std::string& aScenePath);
    static bool SaveSceneLighting(const std::string& aScenePath, const GameDebugSettings::LightingSettings& someSettings);
    static std::string NormalizeScenePath(const std::string& aScenePath);
    static const std::string& GetLastStatusMessage();

private:
    GameSettingsService() = delete;
};
