#pragma once

#include <CommonUtilities/Vector3.hpp>

#include <cstdint>

class GameObject;

enum class CombatTeam
{
    Neutral,
    Player,
    Enemy
};

struct HitboxSpawnRequest
{
    CombatTeam team = CombatTeam::Neutral;
    GameObject* owner = nullptr;
    CommonUtilities::Vector3<float> center = { 0.0f, 0.0f, 0.0f };
    CommonUtilities::Vector3<float> size = { 0.0f, 0.0f, 0.0f };
    float lifetimeSeconds = 0.0f;
    std::int32_t damage = 0;
};

struct HurtboxDescriptor
{
    CombatTeam team = CombatTeam::Neutral;
    GameObject* owner = nullptr;
    CommonUtilities::Vector3<float> center = { 0.0f, 0.0f, 0.0f };
    CommonUtilities::Vector3<float> size = { 0.0f, 0.0f, 0.0f };
};
