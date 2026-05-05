#pragma once

#include "CombatInterfaces.h"

#include <memory>
#include <unordered_set>
#include <vector>

class GameObject;

class CombatSystem final
{
public:
    std::uint64_t StartAttack(const AttackData& anAttack);
    void Update(float aDeltaTime, std::vector<std::unique_ptr<GameObject>>& someObjects);

    const std::vector<HitEvent>& GetHitEventsThisFrame() const;

private:
    struct ActiveAttack
    {
        std::uint64_t id = 0;
        AttackData data;
        float remainingSeconds = 0.0f;
        std::unordered_set<std::uint64_t> hitTargets;
    };

    std::vector<ActiveAttack> myActiveAttacks;
    std::vector<HitEvent> myHitEventsThisFrame;
    std::uint64_t myNextAttackId = 1;
};

class CombatService final
{
public:
    static void Set(CombatSystem* aSystem);
    static CombatSystem* Get();
    static std::uint64_t StartAttack(const AttackData& anAttack);

private:
    static CombatSystem* ourSystem;
};
