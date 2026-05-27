#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <any>

#include <CommonUtilities/Vector2.hpp>
#include <CommonUtilities/Vector3.hpp>
#include <CommonUtilities/Quaternion.hpp>

struct SceneSpriteData
{
    static constexpr int kTextureSlotCount = 4;

    std::string texturePath;
    std::array<std::string, kTextureSlotCount> texturePaths = {};
    CommonUtilities::Vector2<float> size = { 100.0f, 100.0f };
    CommonUtilities::Vector2<float> pivot = { 0.5f, 0.5f };
};

/// Holds all properties read from an editor scene object.
/// This data is passed to the GameObjectFactory for instantiation.
struct SceneObjectData
{
    /// Name of the object as set in the editor.
    std::string name;

    /// Name of object definition in the editor.
    std::string ObjDefinition;

    /// Stable type identifier that maps to a prefab at Objects/{typeId}.tgo.
    std::string typeId;

    /// World position from the editor transform.
    CommonUtilities::Vector3<float> position;

    /// World rotation from the editor transform.
    CommonUtilities::Quaternion<float> rotation;

    /// World scale from the editor transform.
    CommonUtilities::Vector3<float> scale;

    /// All custom properties defined in the editor, keyed by property name.
    /// Use TryGetProperty<T>() to retrieve typed values.
    /// 
    /// Common property types:
    /// - float, int, bool, std::string
    /// - Vector3f for positions/directions
    std::unordered_map<std::string, std::any> properties;

    /// Try to get a custom property by name with the specified type.
    /// Returns true if the property exists and has the correct type.
    template <typename T>
    bool TryGetProperty(const std::string& aName, T& outValue) const
    {
        auto it = properties.find(aName);
        if (it == properties.end())
        {
            return false;
        }

        try
        {
            outValue = std::any_cast<T>(it->second);
            return true;
        }
        catch (const std::bad_any_cast&)
        {
            return false;
        }
    }

    /// Get a custom property by name with a default value if not found.
    template <typename T>
    T GetPropertyOr(const std::string& aName, const T& aDefault) const
    {
        T value;
        if (TryGetProperty(aName, value))
        {
            return value;
        }
        return aDefault;
    }

    /// Check if a custom property exists.
    bool HasProperty(const std::string& aName) const
    {
        return properties.find(aName) != properties.end();
    }
};

