#pragma once

#include <tge/graphics/AmbientLight.h>
#include <tge/render/RenderCommon.h>

namespace GameDebugSettings
{
    
#pragma region  Lighting
    struct LightingSettings
    {
        bool enableDirectionalLight = true;
        float directionalRotationDegrees[3] = { 225.0f, -45.0f, 0.0f };
        float directionalColor[3] = { 1.0f, 1.0f, 1.0f };
        float directionalIntensity = 1.0f;
        float directionalSoftness = 0.1f;

        bool enableAmbientLight = true;
        Tga::AmbientLightType ambientMode = Tga::AmbientLightType::Uniform;
        float ambientColor[3] = { 0.5f, 0.5f, 0.5f };
        float ambientIntensity = 1.0f;
    };

    inline LightingSettings MakeDefaultLightingSettings()
    {
        return {};
    }

    inline LightingSettings& CurrentLightingSettings()
    {
        static LightingSettings settings = MakeDefaultLightingSettings();
        return settings;
    }

    inline void SetLightingSettings(const LightingSettings& someSettings)
    {
        CurrentLightingSettings() = someSettings;
    }

    inline void ResetLightingSettingsToDefaults()
    {
        SetLightingSettings(MakeDefaultLightingSettings());
    }
#pragma endregion

    inline bool& ShowColliderDebugLines()
    {
        static bool value = false;
        return value;
    }

    inline bool& ShowCombatHitboxes()
    {
        static bool value = false;
        return value;
    }

    inline bool& ShowEnemyAvoidanceDebugLines()
    {
        static bool value = false;
        return value;
    }

    inline bool& EnableCollisionDebugLog()
    {
        static bool value = false;
        return value;
    }

    inline bool& LogCollisionPairChecks()
    {
        static bool value = false;
        return value;
    }

    inline bool& LogCollisionResolutionDetails()
    {
        static bool value = false;
        return value;
    }

    inline int& MaxCollisionDebugLogsPerFrame()
    {
        static int value = 80;
        return value;
    }

    inline bool& EnableColliderDrawerDebugLog()
    {
        static bool value = false;
        return value;
    }

    inline int& MaxColliderDrawerDebugLogsPerFrame()
    {
        static int value = 20;
        return value;
    }

    inline bool& ShowVfxDebugLines()
    {
        static bool value = false;
        return value;
    }

    inline bool& ShowVfxPivotMarker()
    {
        static bool value = true;
        return value;
    }
}
