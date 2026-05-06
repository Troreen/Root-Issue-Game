#pragma once

#include "CollisionShapeType.h"
#include "ObjectLayer.h"

#include <CommonUtilities/Vector3.hpp>

#include <cstdint>

class GameObject;

enum class CombatTeam
{
    Neutral,
    Player,
    Enemy
};

enum class AttackType
{
    MeleeLight,
    MeleeCombo,
    MeleeCharged,
    DodgeAttack,
    Ranged,
    EnemyMelee,
    EnemyRoll
};

struct CombatTargetMask
{
    std::uint32_t bits = 0;

    void AddLayer(ObjectLayer aLayer)
    {
        bits |= 1u << static_cast<std::uint32_t>(aLayer);
    }

    bool Contains(ObjectLayer aLayer) const
    {
        return (bits & (1u << static_cast<std::uint32_t>(aLayer))) != 0u;
    }
};

struct AttackData
{
    CombatTeam team = CombatTeam::Neutral;
    GameObject* owner = nullptr;
    AttackType type = AttackType::MeleeLight;
    CollisionShapeType collisionShape = CollisionShapeType::Sphere;
    CombatTargetMask targetLayers;
    CommonUtilities::Vector3<float> localCenterOffset = { 0.0f, 0.0f, 0.0f };
    CommonUtilities::Vector3<float> size = { 0.0f, 0.0f, 0.0f };
    float radius = 0.0f;
    float activeDurationSeconds = 0.0f;
    float knockbackStrength = 0.0f;
    std::int32_t damage = 0;
    bool onlyHitForwardHemisphere = true;
};

struct HitEvent
{
    std::uint64_t attackId = 0;
    GameObject* attacker = nullptr;
    GameObject* target = nullptr;
    CommonUtilities::Vector3<float> knockback = { 0.0f, 0.0f, 0.0f };
    AttackType type = AttackType::MeleeLight;
    std::int32_t damage = 0;
};

inline AttackData CreateEnemyMeleeAttackData(GameObject& anEnemy, std::int32_t aDamage)
{
    AttackData attack;
    attack.owner = &anEnemy;
    attack.team = CombatTeam::Enemy;
    attack.type = AttackType::EnemyMelee;
    attack.collisionShape = CollisionShapeType::Sphere;
    attack.damage = aDamage > 0 ? aDamage : 1;
    attack.localCenterOffset = CommonUtilities::Vector3<float>(0.0f, 90.0f, 0.0f);
    attack.radius = 160.0f;
    attack.activeDurationSeconds = 0.16f;
    attack.knockbackStrength = 250.0f;
    attack.onlyHitForwardHemisphere = true;
    attack.targetLayers.AddLayer(ObjectLayer::Player);
    return attack;
}

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
