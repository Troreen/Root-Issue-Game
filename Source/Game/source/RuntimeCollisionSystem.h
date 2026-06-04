#pragma once

#include "CollisionTypes.h"
#include "RuntimeCollisionTypes.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

class GameObject;

class RuntimeCollisionSystem final
{
public:
    void Run(std::vector<std::unique_ptr<GameObject>>& someGameObjects);
    void AuditRequiredColliders(const std::vector<std::unique_ptr<GameObject>>& someGameObjects) const;

    bool Raycast(
        const std::vector<std::unique_ptr<GameObject>>& someGameObjects,
        const CollisionRaycastQuery& aQuery,
        CollisionRaycastHit& outHit) const;

    const std::vector<CollisionContact>& GetContacts() const;

private:
    std::vector<CollisionContact> myContacts;
    std::unordered_map<std::uint64_t, RuntimeCollision::CollisionPairState> myCollisionPairsLastFrame;
    std::unordered_map<std::uint64_t, CommonUtilities::Vector3<float>> myPreviousColliderPositionsById;
};

class RuntimeCollisionService final
{
public:
    static void Set(RuntimeCollisionSystem* aSystem, std::vector<std::unique_ptr<GameObject>>* someGameObjects);
    static void Clear();
    static bool Raycast(const CollisionRaycastQuery& aQuery, CollisionRaycastHit& outHit);

private:
    static RuntimeCollisionSystem* ourSystem;
    static std::vector<std::unique_ptr<GameObject>>* ourGameObjects;
};
