#pragma once

#include "CollisionTypes.h"

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

    const std::vector<CollisionContact>& GetContacts() const;

private:
    struct CollisionPairState
    {
        std::uint64_t firstId = 0;
        std::uint64_t secondId = 0;
        GameObject* first = nullptr;
        GameObject* second = nullptr;
    };

    std::vector<CollisionContact> myContacts;
    std::unordered_map<std::uint64_t, CollisionPairState> myCollisionPairsLastFrame;
};
