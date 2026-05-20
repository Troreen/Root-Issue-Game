#pragma once

#include "GameObject.h"
#include "ObjectLayer.h"

#include <CommonUtilities/Vector3.hpp>

#include <cstdint>
#include <string>

enum class CollisionPhase
{
    Enter,
    Stay,
    Exit
};

enum class CollisionRule
{
    Ignore,
    Block,
    Trigger
};

struct CollisionContact
{
    GameObject* first = nullptr;
    GameObject* second = nullptr;
    CommonUtilities::Vector3<float> normal = { 0.0f, 0.0f, 0.0f };
    float penetration = 0.0f;
    CollisionPhase phase = CollisionPhase::Stay;

    bool Involves(const GameObject& anObject) const
    {
        return first == &anObject || second == &anObject;
    }

    GameObject* GetOther(const GameObject& anObject) const
    {
        if (first == &anObject)
        {
            return second;
        }

        if (second == &anObject)
        {
            return first;
        }

        return nullptr;
    }

    bool InvolvesLayer(ObjectLayer aLayer) const
    {
        return (first && first->GetLayer() == aLayer) ||
            (second && second->GetLayer() == aLayer);
    }
};

struct CollisionObjectInfo
{
    GameObject* object = nullptr;
    ObjectLayer layer = ObjectLayer::WorldStatic;
    std::uint64_t collisionId = 0;
    std::string name;

    bool IsValid() const
    {
        return object != nullptr;
    }

    bool IsLayer(ObjectLayer aLayer) const
    {
        return IsValid() && layer == aLayer;
    }
};

struct CollisionLayerMask
{
    std::uint32_t bits = 0u;

    static CollisionLayerMask FromLayer(ObjectLayer aLayer)
    {
        CollisionLayerMask mask;
        mask.AddLayer(aLayer);
        return mask;
    }

    void AddLayer(ObjectLayer aLayer)
    {
        bits |= 1u << static_cast<std::uint32_t>(aLayer);
    }

    bool Contains(ObjectLayer aLayer) const
    {
        return (bits & (1u << static_cast<std::uint32_t>(aLayer))) != 0u;
    }
};

struct CollisionRaycastQuery
{
    CommonUtilities::Vector3<float> origin = { 0.0f, 0.0f, 0.0f };
    CommonUtilities::Vector3<float> direction = { 0.0f, 0.0f, 1.0f };
    float maxDistance = 0.0f;
    CollisionLayerMask layers;
    GameObject* ignoredObject = nullptr;
    bool includeTriggerColliders = true;
};

struct CollisionRaycastHit
{
    GameObject* object = nullptr;
    ObjectLayer layer = ObjectLayer::WorldStatic;
    std::uint64_t collisionId = 0;
    std::string name;
    CommonUtilities::Vector3<float> point = { 0.0f, 0.0f, 0.0f };
    CommonUtilities::Vector3<float> normal = { 0.0f, 0.0f, 0.0f };
    float distance = 0.0f;

    bool HasHit() const
    {
        return object != nullptr;
    }

    CollisionObjectInfo GetObjectInfo() const
    {
        return { object, layer, collisionId, name };
    }
};
