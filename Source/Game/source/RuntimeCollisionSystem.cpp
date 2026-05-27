#include "RuntimeCollisionSystem.h"

#include "CollisionLayerRules.h"
#include "DebugSettings.h"
#include "GameObject.h"
#include "ObjectLayer.h"
#include "RuntimeCollisionContacts.h"
#include "RuntimeCollisionDebug.h"
#include "RuntimeCollisionLayers.h"
#include "RuntimeCollisionNarrowPhase.h"
#include "RuntimeCollisionRaycasts.h"
#include "RuntimeCollisionShapes.h"

#include <CommonUtilities/Vector3.hpp>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

using Vector3f = CommonUtilities::Vector3<float>;
using namespace RuntimeCollision;

void RuntimeCollisionSystem::Run(std::vector<std::unique_ptr<GameObject>>& someGameObjects)
{
    const CollisionLayerRuleTable rules = BuildCollisionRules();
    myContacts.clear();

    const bool logCollisionDebug = GameDebugSettings::EnableCollisionDebugLog();
    const bool logPairChecks = GameDebugSettings::LogCollisionPairChecks();
    const bool logResolutionDetails = GameDebugSettings::LogCollisionResolutionDetails();
    int collisionDebugLogBudget = (std::max)(1, GameDebugSettings::MaxCollisionDebugLogsPerFrame());
    int skippedCollisionDebugLogs = 0;
    static std::uint64_t collisionDebugFrameIndex = 0;
    ++collisionDebugFrameIndex;

    auto logCollisionDebugLine = [&](const std::string& aText)
        {
            if (!logCollisionDebug)
            {
                return;
            }

            if (collisionDebugLogBudget <= 0)
            {
                ++skippedCollisionDebugLogs;
                return;
            }

            --collisionDebugLogBudget;
            std::cout << "[CollisionDebug][frame " << collisionDebugFrameIndex << "] " << aText << "\n";
        };

    std::unordered_map<std::uint64_t, CollisionPairState> collisionPairsThisFrame;
    CollisionObjectLayers collisionLayers = CollectCollisionObjectLayers(someGameObjects);
    auto& liveColliderObjectsById = collisionLayers.liveColliderObjectsById;
    auto& playerObjects = collisionLayers.playerObjects;
    auto& enemyObjects = collisionLayers.enemyObjects;
    auto& worldStaticObjects = collisionLayers.worldStaticObjects;
    auto& worldDamageObjects = collisionLayers.worldDamageObjects;
    auto& triggerObjects = collisionLayers.triggerObjects;
    auto& pickupObjects = collisionLayers.pickupObjects;
    auto& switchObjects = collisionLayers.switchObjects;
    auto& bulletObjects = collisionLayers.bulletObjects;

    for (auto& [id, object] : liveColliderObjectsById)
    {
        (void)id;
        RefreshRuntimeCollider(*object);
    }

    if (logCollisionDebug)
    {
        std::ostringstream stream;
        stream << "live=" << liveColliderObjectsById.size()
            << " players=" << playerObjects.size()
            << " enemies=" << enemyObjects.size()
            << " worldStatic=" << worldStaticObjects.size()
            << " worldDestruct=" << worldDamageObjects.size()
            << " triggers=" << triggerObjects.size()
            << " pickups=" << pickupObjects.size();
        logCollisionDebugLine(stream.str());
    }

    auto registerContact = [&](GameObject* aFirst, GameObject* aSecond, const Vector3f& aNormal, const float aPenetration)
        {
            CollisionContact* contact = RegisterContact(
                *aFirst,
                *aSecond,
                aNormal,
                aPenetration,
                myCollisionPairsLastFrame,
                collisionPairsThisFrame,
                myContacts);

            if (!contact)
            {
                return;
            }

            if (logCollisionDebug && (logResolutionDetails || contact->phase != CollisionPhase::Stay))
            {
                std::ostringstream stream;
                stream << "contact phase=" << ToPhaseName(contact->phase)
                    << " normal=" << ToString(aNormal)
                    << " penetration=" << aPenetration
                    << " first=" << DescribeObject(*aFirst)
                    << " second=" << DescribeObject(*aSecond);
                logCollisionDebugLine(stream.str());
            }
        };

    auto resolveBlock = [&](GameObject& aDynamicObject, GameObject& anObstacleObject, const bool aRegisterContact)
        {
            const std::uint64_t pairKey = MakeCollisionPairKey(aDynamicObject, anObstacleObject);
            const bool isNewPair = myCollisionPairsLastFrame.find(pairKey) == myCollisionPairsLastFrame.end();
            const Vector3f originBefore = aDynamicObject.GetTransform().GetPosition();
            const CollisionShape dynamicShape = GetCollisionShape(aDynamicObject);
            const CollisionShape obstacleShape = GetCollisionShape(anObstacleObject);
            const CommonUtilities::AABB3D<float> dynamicAabb = dynamicShape.bounds;
            const CommonUtilities::AABB3D<float> obstacleAabb = obstacleShape.bounds;
            const Vector3f overlaps = GetOverlapDepths(dynamicAabb, obstacleAabb);
            Vector3f separation;
            Vector3f normal;
            float penetration = 0.0f;
            if (!TryComputeShapeSeparation(dynamicShape, obstacleShape, separation, normal, penetration))
            {
                if (logCollisionDebug && logPairChecks)
                {
                    std::ostringstream stream;
                    stream << "block miss rule=" << ToRuleName(CollisionRule::Block)
                        << " dynamic=" << DescribeObject(aDynamicObject)
                        << " obstacle=" << DescribeObject(anObstacleObject)
                        << " dynamicAabb={" << DescribeAabb(dynamicAabb) << "}"
                        << " obstacleAabb={" << DescribeAabb(obstacleAabb) << "}"
                        << " overlapDepths=" << ToString(overlaps);
                    logCollisionDebugLine(stream.str());
                }
                return false;
            }

            aDynamicObject.GetTransform().Translate(separation);
            RefreshRuntimeCollider(aDynamicObject);

            if (logCollisionDebug && (logResolutionDetails || isNewPair))
            {
                const CommonUtilities::AABB3D<float> resolvedAabb = GetCollisionShape(aDynamicObject).bounds;
                std::ostringstream stream;
                stream << "block hit"
                    << " dynamic=" << DescribeObject(aDynamicObject)
                    << " obstacle=" << DescribeObject(anObstacleObject)
                    << " originBefore=" << ToString(originBefore)
                    << " originAfter=" << ToString(aDynamicObject.GetTransform().GetPosition())
                    << " dynamicAabbBefore={" << DescribeAabb(dynamicAabb) << "}"
                    << " dynamicAabbAfter={" << DescribeAabb(resolvedAabb) << "}"
                    << " obstacleAabb={" << DescribeAabb(obstacleAabb) << "}"
                    << " overlapDepths=" << ToString(overlaps)
                    << " separation=" << ToString(separation)
                    << " normal=" << ToString(normal)
                    << " penetration=" << penetration;
                logCollisionDebugLine(stream.str());
            }

            if (aRegisterContact)
            {
                registerContact(&aDynamicObject, &anObstacleObject, normal, penetration);
            }

            return true;
        };

    auto registerTrigger = [&](GameObject& aFirst, GameObject& aSecond)
        {
            const CollisionShape firstShape = GetCollisionShape(aFirst);
            const CollisionShape secondShape = GetCollisionShape(aSecond);
            const CommonUtilities::AABB3D<float> firstAabb = firstShape.bounds;
            const CommonUtilities::AABB3D<float> secondAabb = secondShape.bounds;
            const Vector3f overlaps = GetOverlapDepths(firstAabb, secondAabb);
            Vector3f separation;
            Vector3f normal;
            float penetration = 0.0f;
            if (!TryComputeShapeSeparation(firstShape, secondShape, separation, normal, penetration))
            {
                if (logCollisionDebug && logPairChecks)
                {
                    std::ostringstream stream;
                    stream << "trigger miss"
                        << " first=" << DescribeObject(aFirst)
                        << " second=" << DescribeObject(aSecond)
                        << " firstAabb={" << DescribeAabb(firstAabb) << "}"
                        << " secondAabb={" << DescribeAabb(secondAabb) << "}"
                        << " overlapDepths=" << ToString(overlaps);
                    logCollisionDebugLine(stream.str());
                }
                return false;
            }

            if (logCollisionDebug)
            {
                std::ostringstream stream;
                stream << "trigger hit"
                    << " first=" << DescribeObject(aFirst)
                    << " second=" << DescribeObject(aSecond)
                    << " firstAabb={" << DescribeAabb(firstAabb) << "}"
                    << " secondAabb={" << DescribeAabb(secondAabb) << "}"
                    << " overlapDepths=" << ToString(overlaps);
                logCollisionDebugLine(stream.str());
            }

            registerContact(&aFirst, &aSecond, Vector3f::Zero, 0.0f);
            return true;
        };

    auto tryProjectileSweep = [&](GameObject& aProjectile, const std::vector<GameObject*>& someTargets)
        {
            const auto previousIt = myPreviousColliderPositionsById.find(aProjectile.GetCollisionId());
            if (previousIt == myPreviousColliderPositionsById.end())
            {
                return;
            }

            const Vector3f origin = previousIt->second;
            const Vector3f current = aProjectile.GetTransform().GetPosition();
            Vector3f delta = current - origin;
            const float distance = delta.Length();
            if (distance <= kCollisionEpsilon)
            {
                return;
            }

            const Vector3f direction = delta / distance;
            const CollisionShape projectileShape = GetCollisionShape(aProjectile);
            const float projectileRadius = GetSweepRadius(projectileShape);

            GameObject* bestTarget = nullptr;
            CollisionRaycastHit bestHit;
            bestHit.distance = distance;

            for (GameObject* target : someTargets)
            {
                if (target == nullptr || !target->IsActive())
                {
                    continue;
                }

                const CollisionRule rule = rules.Get(aProjectile.GetLayer(), target->GetLayer());
                if (rule == CollisionRule::Ignore)
                {
                    continue;
                }

                CollisionRaycastHit hit;
                if (!TryRaycastShape(
                    origin,
                    direction,
                    distance,
                    GetCollisionShape(*target),
                    projectileRadius,
                    hit))
                {
                    continue;
                }

                if (bestTarget == nullptr || hit.distance < bestHit.distance)
                {
                    bestTarget = target;
                    bestHit = hit;
                }
            }

            if (bestTarget == nullptr)
            {
                return;
            }

            aProjectile.GetTransform().SetPosition(bestHit.point);
            RefreshRuntimeCollider(aProjectile);
            registerContact(bestTarget, &aProjectile, bestHit.normal, 0.0f);

            if (bestTarget->GetLayer() == ObjectLayer::WorldStatic)
            {
                aProjectile.DisableAllComponents();
                aProjectile.SetActive(false);
            }
            else if (bestTarget->GetLayer() == ObjectLayer::WorldDamageable)
            {
                aProjectile.DisableAllComponents();
                aProjectile.SetActive(false);
            }
        };

    std::vector<GameObject*> projectileSweepTargets;
    projectileSweepTargets.reserve(worldStaticObjects.size() + worldDamageObjects.size() + switchObjects.size() + enemyObjects.size());
    projectileSweepTargets.insert(projectileSweepTargets.end(), worldStaticObjects.begin(), worldStaticObjects.end());
    projectileSweepTargets.insert(projectileSweepTargets.end(), worldDamageObjects.begin(), worldDamageObjects.end());
    projectileSweepTargets.insert(projectileSweepTargets.end(), switchObjects.begin(), switchObjects.end());
    projectileSweepTargets.insert(projectileSweepTargets.end(), enemyObjects.begin(), enemyObjects.end());

    for (GameObject* bullet : bulletObjects)
    {
        if (bullet && bullet->IsActive())
        {
            tryProjectileSweep(*bullet, projectileSweepTargets);
        }
    }

    for (GameObject* player : playerObjects)
    {
        for (int iteration = 0; iteration < kMaxCollisionIterations; ++iteration)
        {
            bool didResolve = false;
            for (GameObject* worldStatic : worldStaticObjects)
            {
                if (HasTriggerCollider(*player) || HasTriggerCollider(*worldStatic))
                {
                    registerTrigger(*player, *worldStatic);
                    continue;
                }

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

    for (GameObject* player : playerObjects)
    {
        for (int iteration = 0; iteration < kMaxCollisionIterations; ++iteration)
        {
            bool didResolve = false;
            for (GameObject* worldDamage : worldDamageObjects)
            {
                if (HasTriggerCollider(*player) || HasTriggerCollider(*worldDamage))
                {
                    registerTrigger(*player, *worldDamage);
                    continue;
                }

                if (rules.Get(player->GetLayer(), worldDamage->GetLayer()) != CollisionRule::Block)
                {
                    continue;
                }

                if (resolveBlock(*player, *worldDamage, true))
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
                if (HasTriggerCollider(*enemy) || HasTriggerCollider(*worldStatic))
                {
                    registerTrigger(*worldStatic, *enemy);
                    //registerTrigger(*enemy, *worldStatic);
                    //continue;
                }

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

    for (GameObject* enemy : enemyObjects)
    {
        for (int iteration = 0; iteration < kMaxCollisionIterations; ++iteration)
        {
            bool didResolve = false;
            for (GameObject* worldDamage : worldDamageObjects)
            {
                if (HasTriggerCollider(*enemy) || HasTriggerCollider(*worldDamage))
                {
                    registerTrigger(*worldDamage, *enemy);
                    //registerTrigger(*enemy, *worldStatic);
                    //continue;
                }

                if (rules.Get(enemy->GetLayer(), worldDamage->GetLayer()) != CollisionRule::Block)
                {
                    continue;
                }

                if (resolveBlock(*enemy, *worldDamage, true))
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
            if (HasTriggerCollider(*player) || HasTriggerCollider(*enemy))
            {
                registerTrigger(*player, *enemy);
                //continue;
            }

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

    for (GameObject* enemy : enemyObjects)
    {
        for (GameObject* bullet : bulletObjects)
        {
            if (!bullet || !bullet->IsActive())
            {
                continue;
            }

            if (rules.Get(enemy->GetLayer(), bullet->GetLayer()) != CollisionRule::Block)
            {
                continue;
            }

            registerTrigger(*enemy, *bullet);
        }

    }

    for (GameObject* switchy : switchObjects)
    {
        for (GameObject* bullet : bulletObjects)
        {
            if (!bullet || !bullet->IsActive())
            {
                continue;
            }

            if (rules.Get(switchy->GetLayer(), bullet->GetLayer()) != CollisionRule::Trigger)
            {
                continue;
            }

            registerTrigger(*switchy, *bullet);
        }
    }

    const std::size_t firstExitContactIndex = AppendExitContacts(
        myCollisionPairsLastFrame,
        collisionPairsThisFrame,
        liveColliderObjectsById,
        myContacts);

    for (std::size_t contactIndex = firstExitContactIndex; contactIndex < myContacts.size(); ++contactIndex)
    {
        const CollisionContact& contact = myContacts[contactIndex];

        if (logCollisionDebug)
        {
            std::ostringstream stream;
            stream << "contact phase=Exit"
                << " first=" << DescribeObject(*contact.first)
                << " second=" << DescribeObject(*contact.second);
            logCollisionDebugLine(stream.str());
        }
    }

    if (logCollisionDebug && skippedCollisionDebugLogs > 0)
    {
        std::cout << "[CollisionDebug][frame " << collisionDebugFrameIndex << "] skipped "
            << skippedCollisionDebugLogs
            << " log lines because Collision Log Cap / Frame was reached\n";
    }

    myCollisionPairsLastFrame = std::move(collisionPairsThisFrame);

    std::unordered_map<std::uint64_t, Vector3f> previousColliderPositions;
    previousColliderPositions.reserve(liveColliderObjectsById.size());
    for (const auto& [id, object] : liveColliderObjectsById)
    {
        if (object != nullptr && object->IsActive())
        {
            previousColliderPositions.emplace(id, object->GetTransform().GetPosition());
        }
    }
    myPreviousColliderPositionsById = std::move(previousColliderPositions);
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

bool RuntimeCollisionSystem::Raycast(
    const std::vector<std::unique_ptr<GameObject>>& someGameObjects,
    const CollisionRaycastQuery& aQuery,
    CollisionRaycastHit& outHit) const
{
    outHit = {};

    if (aQuery.maxDistance <= 0.0f || aQuery.direction.LengthSqr() <= kCollisionEpsilon)
    {
        return false;
    }

    const Vector3f direction = aQuery.direction.GetNormalized();
    bool didHit = false;
    CollisionRaycastHit bestHit;
    bestHit.distance = aQuery.maxDistance;

    for (const auto& object : someGameObjects)
    {
        if (!object || !object->IsActive() || object.get() == aQuery.ignoredObject)
        {
            continue;
        }

        if (!aQuery.layers.Contains(object->GetLayer()) || !HasRuntimeCollider(*object))
        {
            continue;
        }

        if (!aQuery.includeTriggerColliders && HasTriggerCollider(*object))
        {
            continue;
        }

        CollisionRaycastHit hit;
        if (!TryRaycastShape(
            aQuery.origin,
            direction,
            aQuery.maxDistance,
            GetCollisionShape(*object),
            0.0f,
            hit))
        {
            continue;
        }

        if (!didHit || hit.distance < bestHit.distance)
        {
            didHit = true;
            bestHit = hit;
            bestHit.object = object.get();
            bestHit.layer = object->GetLayer();
            bestHit.collisionId = object->GetCollisionId();
            bestHit.name = object->GetName();
        }
    }

    if (!didHit)
    {
        return false;
    }

    outHit = bestHit;
    return true;
}

const std::vector<CollisionContact>& RuntimeCollisionSystem::GetContacts() const
{
    return myContacts;
}
