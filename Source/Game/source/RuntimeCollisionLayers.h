#pragma once

#include "CollisionLayerRules.h"
#include "RuntimeCollisionTypes.h"

#include <memory>
#include <unordered_map>
#include <vector>

class GameObject;

namespace RuntimeCollision
{
    struct CollisionObjectLayers
    {
        std::unordered_map<std::uint64_t, GameObject*> liveColliderObjectsById;
        std::vector<GameObject*> playerObjects;
        std::vector<GameObject*> enemyObjects;
        std::vector<GameObject*> worldStaticObjects;
        std::vector<GameObject*> worldDamageObjects;
        std::vector<GameObject*> triggerObjects;
        std::vector<GameObject*> pickupObjects;
        std::vector<GameObject*> switchObjects;
        std::vector<GameObject*> bulletObjects;
    };

    CollisionLayerRuleTable BuildCollisionRules();
    CollisionObjectLayers CollectCollisionObjectLayers(std::vector<std::unique_ptr<GameObject>>& someGameObjects);
    bool RequiresColliderForAudit(ObjectLayer aLayer);
    const char* ToLayerName(ObjectLayer aLayer);
    const char* ToRuleName(CollisionRule aRule);
    const char* ToPhaseName(CollisionPhase aPhase);
}
