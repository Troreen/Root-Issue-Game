#include "RuntimeCollisionSystem.h"

#include "BoxColliderComponent.h"
#include "CollisionLayerRules.h"
#include "GameObject.h"
#include "ObjectLayer.h"
#include "SphereColliderComponent.h"

#include <CommonUtilities/Vector3.hpp>

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

using Vector3f = CommonUtilities::Vector3<float>;

namespace
{
    constexpr int kMaxCollisionIterations = 4;

    const char* ToLayerName(ObjectLayer aLayer)
    {
        switch (aLayer)
        {
        case ObjectLayer::WorldStatic:
            return "WorldStatic";
        case ObjectLayer::Player:
            return "Player";
        case ObjectLayer::BasicMeleeEnemy:
            return "BasicMeleeEnemy";
        case ObjectLayer::Projectile:
            return "Projectile";
        case ObjectLayer::Trigger:
            return "Trigger";
        case ObjectLayer::Pickup:
            return "Pickup";
        case ObjectLayer::NPC:
            return "NPC";
        case ObjectLayer::Count:
        default:
            return "Unknown";
        }
    }

    bool HasRuntimeCollider(const GameObject& anObject)
    {
        return anObject.GetComponent<BoxColliderComponent>() != nullptr ||
            anObject.GetComponent<SphereColliderComponent>() != nullptr;
    }

    void RefreshRuntimeCollider(GameObject& anObject)
    {
        if (auto* box = anObject.GetComponent<BoxColliderComponent>())
        {
            box->Update(0.0f);
        }
        else if (auto* sphere = anObject.GetComponent<SphereColliderComponent>())
        {
            sphere->Update(0.0f);
        }
    }

    std::uint64_t MakeCollisionPairKey(const GameObject& aFirst, const GameObject& aSecond)
    {
        const std::uint64_t low = (std::min)(aFirst.GetCollisionId(), aSecond.GetCollisionId());
        const std::uint64_t high = (std::max)(aFirst.GetCollisionId(), aSecond.GetCollisionId());
        return low ^ (high + 0x9e3779b97f4a7c15ULL + (low << 6) + (low >> 2));
    }

    bool TryCompute3DSeparation(
        const CommonUtilities::AABB3D<float>& aDynamicAabb,
        const CommonUtilities::AABB3D<float>& anObstacleAabb,
        Vector3f& outSeparation,
        Vector3f& outNormal,
        float& outPenetration)
    {
        const float overlapX =
            (std::min)(aDynamicAabb.GetMax().x, anObstacleAabb.GetMax().x) -
            (std::max)(aDynamicAabb.GetMin().x, anObstacleAabb.GetMin().x);
        const float overlapZ =
            (std::min)(aDynamicAabb.GetMax().z, anObstacleAabb.GetMax().z) -
            (std::max)(aDynamicAabb.GetMin().z, anObstacleAabb.GetMin().z);
        const float overlapY =
            (std::min)(aDynamicAabb.GetMax().y, anObstacleAabb.GetMax().y) -
            (std::max)(aDynamicAabb.GetMin().y, anObstacleAabb.GetMin().y);

        if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f)
        {
            return false;
        }

        const Vector3f dynamicCenter = (aDynamicAabb.GetMin() + aDynamicAabb.GetMax()) * 0.5f;
        const Vector3f obstacleCenter = (anObstacleAabb.GetMin() + anObstacleAabb.GetMax()) * 0.5f;

        outSeparation = Vector3f::Zero;
        outNormal = Vector3f::Zero;

        if (overlapX <= overlapY && overlapX <= overlapZ)
        {
            outPenetration = overlapX;
            const float direction = dynamicCenter.x < obstacleCenter.x ? -1.0f : 1.0f;
            outSeparation.x = outPenetration * direction;
            outNormal.x = direction;
        }
        else if (overlapY <= overlapZ)
        {
            outPenetration = overlapY;
            const float direction = dynamicCenter.y < obstacleCenter.y ? -1.0f : 1.0f;
            outSeparation.y = outPenetration * direction;
            outNormal.y = direction;
        }
        else
        {
            outPenetration = overlapZ;
            const float direction = dynamicCenter.z < obstacleCenter.z ? -1.0f : 1.0f;
            outSeparation.z = outPenetration * direction;
            outNormal.z = direction;
        }

        return true;
    }

    bool Has3DOverlap(
        const CommonUtilities::AABB3D<float>& aFirstAabb,
        const CommonUtilities::AABB3D<float>& aSecondAabb)
    {
        const float overlapX =
            (std::min)(aFirstAabb.GetMax().x, aSecondAabb.GetMax().x) -
            (std::max)(aFirstAabb.GetMin().x, aSecondAabb.GetMin().x);
        const float overlapZ =
            (std::min)(aFirstAabb.GetMax().z, aSecondAabb.GetMax().z) -
            (std::max)(aFirstAabb.GetMin().z, aSecondAabb.GetMin().z);
        const float overlapY =
            (std::min)(aFirstAabb.GetMax().y, aSecondAabb.GetMax().y) -
            (std::max)(aFirstAabb.GetMin().y, aSecondAabb.GetMin().y);

        return overlapX > 0.0f && overlapY > 0.0f && overlapZ > 0.0f;
    }

    CollisionLayerRuleTable BuildCollisionRules()
    {
        CollisionLayerRuleTable rules;
        rules.SetSymmetric(ObjectLayer::Player, ObjectLayer::WorldStatic, CollisionRule::Block);
        rules.SetSymmetric(ObjectLayer::BasicMeleeEnemy, ObjectLayer::WorldStatic, CollisionRule::Block);
        rules.SetSymmetric(ObjectLayer::Player, ObjectLayer::BasicMeleeEnemy, CollisionRule::Block);
        rules.SetSymmetric(ObjectLayer::Player, ObjectLayer::Trigger, CollisionRule::Trigger);
        rules.SetSymmetric(ObjectLayer::Player, ObjectLayer::Pickup, CollisionRule::Trigger);
        return rules;
    }

    bool RequiresColliderForAudit(ObjectLayer aLayer)
    {
        return aLayer == ObjectLayer::WorldStatic ||
            aLayer == ObjectLayer::Player ||
            aLayer == ObjectLayer::BasicMeleeEnemy;
    }
}

void RuntimeCollisionSystem::Run(std::vector<std::unique_ptr<GameObject>>& someGameObjects)
{
    const CollisionLayerRuleTable rules = BuildCollisionRules();
    myContacts.clear();

    std::unordered_map<std::uint64_t, CollisionPairState> collisionPairsThisFrame;
    std::unordered_map<std::uint64_t, GameObject*> liveColliderObjectsById;
    std::vector<GameObject*> playerObjects;
    std::vector<GameObject*> enemyObjects;
    std::vector<GameObject*> worldStaticObjects;
    std::vector<GameObject*> triggerObjects;
    std::vector<GameObject*> pickupObjects;

    playerObjects.reserve(someGameObjects.size());
    enemyObjects.reserve(someGameObjects.size());
    worldStaticObjects.reserve(someGameObjects.size());
    triggerObjects.reserve(someGameObjects.size());
    pickupObjects.reserve(someGameObjects.size());

    for (auto& object : someGameObjects)
    {
        if (!object || !object->IsActive() || !HasRuntimeCollider(*object))
        {
            continue;
        }

        liveColliderObjectsById.emplace(object->GetCollisionId(), object.get());

        switch (object->GetLayer())
        {
        case ObjectLayer::Player:
            playerObjects.push_back(object.get());
            break;
        case ObjectLayer::BasicMeleeEnemy:
            enemyObjects.push_back(object.get());
            break;
        case ObjectLayer::WorldStatic:
            worldStaticObjects.push_back(object.get());
            break;
        case ObjectLayer::Trigger:
            triggerObjects.push_back(object.get());
            break;
        case ObjectLayer::Pickup:
            pickupObjects.push_back(object.get());
            break;
        default:
            break;
        }
    }

    for (auto& [id, object] : liveColliderObjectsById)
    {
        (void)id;
        RefreshRuntimeCollider(*object);
    }

    auto registerContact = [&](GameObject* aFirst, GameObject* aSecond, const Vector3f& aNormal, const float aPenetration)
        {
            const std::uint64_t pairKey = MakeCollisionPairKey(*aFirst, *aSecond);
            if (collisionPairsThisFrame.find(pairKey) != collisionPairsThisFrame.end())
            {
                return;
            }

            CollisionPairState pairState;
            pairState.firstId = aFirst->GetCollisionId();
            pairState.secondId = aSecond->GetCollisionId();
            pairState.first = aFirst;
            pairState.second = aSecond;
            collisionPairsThisFrame.emplace(pairKey, pairState);

            CollisionContact contact;
            contact.first = aFirst;
            contact.second = aSecond;
            contact.normal = aNormal;
            contact.penetration = aPenetration;
            contact.phase = myCollisionPairsLastFrame.find(pairKey) != myCollisionPairsLastFrame.end()
                ? CollisionPhase::Stay
                : CollisionPhase::Enter;
            myContacts.push_back(contact);
        };

    auto resolveBlock = [&](GameObject& aDynamicObject, GameObject& anObstacleObject, const bool aRegisterContact)
        {
            const CommonUtilities::AABB3D<float> dynamicAabb = aDynamicObject.GetHitbox();
            const CommonUtilities::AABB3D<float> obstacleAabb = anObstacleObject.GetHitbox();
            if (!Has3DOverlap(dynamicAabb, obstacleAabb))
            {
                return false;
            }

            Vector3f separation;
            Vector3f normal;
            float penetration = 0.0f;
            if (!TryCompute3DSeparation(dynamicAabb, obstacleAabb, separation, normal, penetration))
            {
                return false;
            }

            aDynamicObject.GetTransform().Translate(separation);
            RefreshRuntimeCollider(aDynamicObject);

            if (aRegisterContact)
            {
                registerContact(&aDynamicObject, &anObstacleObject, normal, penetration);
            }

            return true;
        };

    auto registerTrigger = [&](GameObject& aFirst, GameObject& aSecond)
        {
            const CommonUtilities::AABB3D<float> firstAabb = aFirst.GetHitbox();
            const CommonUtilities::AABB3D<float> secondAabb = aSecond.GetHitbox();
            if (!Has3DOverlap(firstAabb, secondAabb))
            {
                return false;
            }

            registerContact(&aFirst, &aSecond, Vector3f::Zero, 0.0f);
            return true;
        };

    for (GameObject* player : playerObjects)
    {
        for (int iteration = 0; iteration < kMaxCollisionIterations; ++iteration)
        {
            bool didResolve = false;
            for (GameObject* worldStatic : worldStaticObjects)
            {
                if (rules.Get(player->GetLayer(), worldStatic->GetLayer()) != CollisionRule::Block)
                {
                    continue;
                }

                if (resolveBlock(*player, *worldStatic, true))
                {
                    didResolve = true;
                }
            }

            if (!didResolve)
            {
                break;
            }
        }
    }

    for (GameObject* enemy : enemyObjects)
    {
        for (int iteration = 0; iteration < kMaxCollisionIterations; ++iteration)
        {
            bool didResolve = false;
            for (GameObject* worldStatic : worldStaticObjects)
            {
                if (rules.Get(enemy->GetLayer(), worldStatic->GetLayer()) != CollisionRule::Block)
                {
                    continue;
                }

                if (resolveBlock(*enemy, *worldStatic, true))
                {
                    didResolve = true;
                }
            }

            if (!didResolve)
            {
                break;
            }
        }
    }

    for (GameObject* player : playerObjects)
    {
        for (GameObject* enemy : enemyObjects)
        {
            if (rules.Get(player->GetLayer(), enemy->GetLayer()) != CollisionRule::Block)
            {
                continue;
            }

            resolveBlock(*player, *enemy, true);
        }
    }

    for (GameObject* player : playerObjects)
    {
        for (GameObject* trigger : triggerObjects)
        {
            if (rules.Get(player->GetLayer(), trigger->GetLayer()) != CollisionRule::Trigger)
            {
                continue;
            }

            registerTrigger(*player, *trigger);
        }

        for (GameObject* pickup : pickupObjects)
        {
            if (rules.Get(player->GetLayer(), pickup->GetLayer()) != CollisionRule::Trigger)
            {
                continue;
            }

            registerTrigger(*player, *pickup);
        }
    }

    for (const auto& [pairKey, pair] : myCollisionPairsLastFrame)
    {
        if (collisionPairsThisFrame.find(pairKey) != collisionPairsThisFrame.end())
        {
            continue;
        }

        auto firstIt = liveColliderObjectsById.find(pair.firstId);
        auto secondIt = liveColliderObjectsById.find(pair.secondId);
        if (firstIt == liveColliderObjectsById.end() || secondIt == liveColliderObjectsById.end())
        {
            continue;
        }

        CollisionContact contact;
        contact.first = firstIt->second;
        contact.second = secondIt->second;
        contact.phase = CollisionPhase::Exit;
        myContacts.push_back(contact);
    }

    myCollisionPairsLastFrame = std::move(collisionPairsThisFrame);
}

void RuntimeCollisionSystem::AuditRequiredColliders(const std::vector<std::unique_ptr<GameObject>>& someGameObjects) const
{
    int warningCount = 0;
    for (const auto& object : someGameObjects)
    {
        if (!object)
        {
            continue;
        }

        const ObjectLayer layer = object->GetLayer();
        if (!RequiresColliderForAudit(layer))
        {
            continue;
        }

        if (HasRuntimeCollider(*object))
        {
            continue;
        }

        ++warningCount;
        std::cout << "[CollisionAudit] WARNING: object '" << object->GetName()
            << "' on layer '" << ToLayerName(layer)
            << "' has no authored collider component.\n";
    }

    if (warningCount > 0)
    {
        std::cout << "[CollisionAudit] Total missing required colliders: " << warningCount << "\n";
    }
}

const std::vector<CollisionContact>& RuntimeCollisionSystem::GetContacts() const
{
    return myContacts;
}
