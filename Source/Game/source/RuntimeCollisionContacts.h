#pragma once

#include "RuntimeCollisionTypes.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

class GameObject;

namespace RuntimeCollision
{
    std::uint64_t MakeCollisionPairKey(const GameObject& aFirst, const GameObject& aSecond);

    CollisionContact* RegisterContact(
        GameObject& aFirst,
        GameObject& aSecond,
        const Vector3f& aNormal,
        float aPenetration,
        const std::unordered_map<std::uint64_t, CollisionPairState>& somePreviousPairs,
        std::unordered_map<std::uint64_t, CollisionPairState>& someCurrentPairs,
        std::vector<CollisionContact>& outContacts);

    std::size_t AppendExitContacts(
        const std::unordered_map<std::uint64_t, CollisionPairState>& somePreviousPairs,
        const std::unordered_map<std::uint64_t, CollisionPairState>& someCurrentPairs,
        const std::unordered_map<std::uint64_t, GameObject*>& someLiveColliderObjectsById,
        std::vector<CollisionContact>& outContacts);
}
