#pragma once

#include <tge/render/RenderCommon.h>

namespace GameDebugSettings
{
    
#pragma region  Lighting
#pragma region Light direction
    inline float& DirectionalLightYawDegrees()
    {
        static float value = 225.0f;
        return value;
    }

    inline float& DirectionalLightPitchDegrees()
    {
        static float value = -45.0f;
        return value;
    }

    inline float& DirectionalLightRollDegrees()
    {
        static float value = 0.0f;
        return value;
    }
#pragma endregion
#pragma region Light color and intensity
    inline float& DirectionalLightColorR()
    {
        static float value = 1.0f;
        return value;
    }

    inline float& DirectionalLightColorG()
    {
        static float value = 1.0f;
        return value;
    }

    inline float& DirectionalLightColorB()
    {
        static float value = 1.0f;
        return value;
    }

    inline float& DirectionalLightIntensity()
    {
        static float value = 0.1f;
        return value;
    }

    inline float& AmbientLightColorR()
    {
        static float value = 0.5f;
        return value;
    }

    inline float& AmbientLightColorG()
    {
        static float value = 0.5f;
        return value;
    }

    inline float& AmbientLightColorB()
    {
        static float value = 0.5f;
        return value;
    }

    inline float& AmbientLightIntensity()
    {
        static float value = 1.0f;
        return value;
    }
#pragma endregion

    inline bool& EnableDirectionalLight()
    {
        static bool value = true;
        return value;
    }

    inline bool& EnableAmbientLight()
    {
        static bool value = true;
        return value;
    }

    inline void ResetLightingSettingsToDefaults()
    {
        DirectionalLightYawDegrees() = 225.0f;
        DirectionalLightPitchDegrees() = -45.0f;
        DirectionalLightRollDegrees() = 0.0f;

        DirectionalLightColorR() = 1.0f;
        DirectionalLightColorG() = 1.0f;
        DirectionalLightColorB() = 1.0f;
        DirectionalLightIntensity() = 0.1f;

        AmbientLightColorR() = 0.5f;
        AmbientLightColorG() = 0.5f;
        AmbientLightColorB() = 0.5f;
        AmbientLightIntensity() = 1.0f;

        EnableDirectionalLight() = true;
        EnableAmbientLight() = true;
    }
#pragma endregion

    inline bool& ShowColliderDebugLines()
    {
        static bool value = false;
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
