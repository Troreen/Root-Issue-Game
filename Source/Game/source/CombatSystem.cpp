#include "CombatSystem.h"

#include "CollisionQuery.h"
#include "DamageableComponent.h"
#include "GameObject.h"
#include "KnockbackComponent.h"

#include <algorithm>
#include <iostream>

namespace
{
    using Vector3f = CommonUtilities::Vector3<float>;

    Vector3f GetAttackCenter(const AttackData& anAttack)
    {
        if (anAttack.owner == nullptr)
        {
            return Vector3f::Zero;
        }

        auto& transform = anAttack.owner->GetTransform();
        return transform.GetPosition()
            + transform.GetRight() * anAttack.localCenterOffset.x
            + Vector3f::UnitY * anAttack.localCenterOffset.y
            + transform.GetForward() * anAttack.localCenterOffset.z;
    }

    Vector3f GetKnockback(GameObject& anAttacker, GameObject& aTarget, float aStrength)
    {
        Vector3f direction = aTarget.GetTransform().GetPosition() - anAttacker.GetTransform().GetPosition();
        direction.y = 0.0f;
        if (direction.LengthSqr() <= 0.001f)
        {
            direction = anAttacker.GetTransform().GetForward();
            direction.y = 0.0f;
        }

        if (direction.LengthSqr() <= 0.001f)
        {
            return Vector3f::Zero;
        }

        return direction.GetNormalized() * aStrength;
    }
}

std::uint64_t CombatSystem::StartAttack(const AttackData& anAttack)
{
    if (anAttack.owner == nullptr || anAttack.activeDurationSeconds <= 0.0f || anAttack.damage <= 0)
    {
        return 0;
    }

    ActiveAttack activeAttack;
    activeAttack.id = myNextAttackId++;
    activeAttack.data = anAttack;
    activeAttack.remainingSeconds = anAttack.activeDurationSeconds;
    myActiveAttacks.push_back(std::move(activeAttack));

    std::cout << "[Combat] Started attack id=" << myActiveAttacks.back().id
        << " owner='" << anAttack.owner->GetName() << "'"
        << " damage=" << anAttack.damage << "\n";

    return myActiveAttacks.back().id;
}

void CombatSystem::Update(float aDeltaTime, std::vector<std::unique_ptr<GameObject>>& someObjects)
{
    myHitEventsThisFrame.clear();

    for (ActiveAttack& attack : myActiveAttacks)
    {
        if (attack.data.owner == nullptr || !attack.data.owner->IsActive())
        {
            attack.remainingSeconds = 0.0f;
            continue;
        }

        const CollisionQuery::Shape attackShape =
            CollisionQuery::MakeBoxShape(GetAttackCenter(attack.data), attack.data.size);

        for (std::unique_ptr<GameObject>& object : someObjects)
        {
            GameObject* target = object.get();
            if (target == nullptr || !target->IsActive() || target == attack.data.owner)
            {
                continue;
            }

            if (!attack.data.targetLayers.Contains(target->GetLayer()) ||
                attack.hitTargets.find(target->GetCollisionId()) != attack.hitTargets.end() ||
                !CollisionQuery::HasRuntimeCollider(*target))
            {
                continue;
            }

            CollisionQuery::RefreshRuntimeCollider(*target);
            Vector3f separation;
            Vector3f normal;
            float penetration = 0.0f;
            if (!CollisionQuery::TryComputeSeparation(
                attackShape,
                CollisionQuery::GetShape(*target),
                separation,
                normal,
                penetration))
            {
                continue;
            }

            attack.hitTargets.insert(target->GetCollisionId());

            const Vector3f knockback = GetKnockback(*attack.data.owner, *target, attack.data.knockbackStrength);
            if (DamageableComponent* damageable = target->GetComponent<DamageableComponent>())
            {
                damageable->TakeDamage(attack.data.damage, attack.data.owner);
            }

            if (KnockbackComponent* knockbackReceiver = target->GetComponent<KnockbackComponent>())
            {
                knockbackReceiver->ApplyImpulse(knockback);
            }

            HitEvent event;
            event.attackId = attack.id;
            event.attacker = attack.data.owner;
            event.target = target;
            event.damage = attack.data.damage;
            event.knockback = knockback;
            event.type = attack.data.type;
            myHitEventsThisFrame.push_back(event);

            std::cout << "[Combat] Hit attackId=" << attack.id
                << " attacker='" << attack.data.owner->GetName() << "'"
                << " target='" << target->GetName() << "'"
                << " damage=" << attack.data.damage << "\n";
        }

        attack.remainingSeconds -= aDeltaTime;
    }

    myActiveAttacks.erase(
        std::remove_if(
            myActiveAttacks.begin(),
            myActiveAttacks.end(),
            [](const ActiveAttack& anAttack)
            {
                return anAttack.remainingSeconds <= 0.0f;
            }),
        myActiveAttacks.end());
}

const std::vector<HitEvent>& CombatSystem::GetHitEventsThisFrame() const
{
    return myHitEventsThisFrame;
}

CombatSystem* CombatService::ourSystem = nullptr;

void CombatService::Set(CombatSystem* aSystem)
{
    ourSystem = aSystem;
}

CombatSystem* CombatService::Get()
{
    return ourSystem;
}

std::uint64_t CombatService::StartAttack(const AttackData& anAttack)
{
    if (ourSystem == nullptr)
    {
        return 0;
    }

    return ourSystem->StartAttack(anAttack);
}
