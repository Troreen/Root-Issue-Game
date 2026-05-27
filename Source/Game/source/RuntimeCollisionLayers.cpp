#include "RuntimeCollisionLayers.h"

#include "GameObject.h"
#include "RuntimeCollisionShapes.h"

namespace RuntimeCollision
{
    const char* ToLayerName(ObjectLayer aLayer)
    {
        switch (aLayer)
        {
        case ObjectLayer::WorldStatic:
            return "WorldStatic";
        case ObjectLayer::WorldDamageable:
            return "WorldDamageable";
        case ObjectLayer::Player:
            return "Player";
        case ObjectLayer::Enemy:
            return "Enemy";
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

    CollisionObjectLayers CollectCollisionObjectLayers(std::vector<std::unique_ptr<GameObject>>& someGameObjects)
    {
        CollisionObjectLayers layers;

        layers.playerObjects.reserve(someGameObjects.size());
        layers.enemyObjects.reserve(someGameObjects.size());
        layers.worldStaticObjects.reserve(someGameObjects.size());
        layers.worldDamageObjects.reserve(someGameObjects.size());
        layers.triggerObjects.reserve(someGameObjects.size());
        layers.pickupObjects.reserve(someGameObjects.size());
        layers.switchObjects.reserve(someGameObjects.size());
        layers.bulletObjects.reserve(someGameObjects.size());

        for (auto& object : someGameObjects)
        {
            if (!object || !object->IsActive() || !HasRuntimeCollider(*object))
            {
                continue;
            }

            layers.liveColliderObjectsById.emplace(object->GetCollisionId(), object.get());

            switch (object->GetLayer())
            {
            case ObjectLayer::Player:
                layers.playerObjects.push_back(object.get());
                break;
            case ObjectLayer::Enemy:
                layers.enemyObjects.push_back(object.get());
                break;
            case ObjectLayer::WorldStatic:
                layers.worldStaticObjects.push_back(object.get());
                break;
            case ObjectLayer::WorldDamageable:
                layers.worldDamageObjects.push_back(object.get());
                break;
            case ObjectLayer::Trigger:
                layers.triggerObjects.push_back(object.get());
                break;
            case ObjectLayer::Pickup:
                layers.pickupObjects.push_back(object.get());
                break;
            case ObjectLayer::Switch:
                layers.switchObjects.push_back(object.get());
                break;
            case ObjectLayer::Projectile:
                layers.bulletObjects.push_back(object.get());
                break;
            default:
                break;
            }
        }

        return layers;
    }

    CollisionLayerRuleTable BuildCollisionRules()
    {
        CollisionLayerRuleTable rules;
        rules.SetSymmetric(ObjectLayer::Player, ObjectLayer::WorldStatic, CollisionRule::Block);
        rules.SetSymmetric(ObjectLayer::Player, ObjectLayer::WorldDamageable, CollisionRule::Block);
        rules.SetSymmetric(ObjectLayer::Enemy, ObjectLayer::WorldStatic, CollisionRule::Block);
        rules.SetSymmetric(ObjectLayer::Enemy, ObjectLayer::WorldDamageable, CollisionRule::Block);
        rules.SetSymmetric(ObjectLayer::Player, ObjectLayer::Enemy, CollisionRule::Block);
        rules.SetSymmetric(ObjectLayer::Player, ObjectLayer::Trigger, CollisionRule::Trigger);
        rules.SetSymmetric(ObjectLayer::Player, ObjectLayer::Pickup, CollisionRule::Trigger);
        rules.SetSymmetric(ObjectLayer::Player, ObjectLayer::Switch, CollisionRule::Block);
        rules.SetSymmetric(ObjectLayer::Projectile, ObjectLayer::WorldStatic, CollisionRule::Block);
        rules.SetSymmetric(ObjectLayer::Projectile, ObjectLayer::WorldDamageable, CollisionRule::Block);
        rules.SetSymmetric(ObjectLayer::Projectile, ObjectLayer::Switch, CollisionRule::Trigger);
        rules.SetSymmetric(ObjectLayer::Projectile, ObjectLayer::Enemy, CollisionRule::Block);
        return rules;
    }

    bool RequiresColliderForAudit(ObjectLayer aLayer)
    {
        return aLayer == ObjectLayer::WorldStatic ||
            aLayer == ObjectLayer::WorldDamageable ||
            aLayer == ObjectLayer::Player ||
            aLayer == ObjectLayer::Enemy;
    }

    const char* ToRuleName(CollisionRule aRule)
    {
        switch (aRule)
        {
        case CollisionRule::Ignore:
            return "Ignore";
        case CollisionRule::Block:
            return "Block";
        case CollisionRule::Trigger:
            return "Trigger";
        default:
            return "Unknown";
        }
    }

    const char* ToPhaseName(CollisionPhase aPhase)
    {
        switch (aPhase)
        {
        case CollisionPhase::Enter:
            return "Enter";
        case CollisionPhase::Stay:
            return "Stay";
        case CollisionPhase::Exit:
            return "Exit";
        default:
            return "Unknown";
        }
    }
}
