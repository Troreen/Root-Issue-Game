#include "RuntimeCollisionContacts.h"

#include "GameObject.h"

#include <algorithm>

namespace RuntimeCollision
{
    std::uint64_t MakeCollisionPairKey(const GameObject& aFirst, const GameObject& aSecond)
    {
        const std::uint64_t low = (std::min)(aFirst.GetCollisionId(), aSecond.GetCollisionId());
        const std::uint64_t high = (std::max)(aFirst.GetCollisionId(), aSecond.GetCollisionId());
        return low ^ (high + 0x9e3779b97f4a7c15ULL + (low << 6) + (low >> 2));
    }

    CollisionContact* RegisterContact(
        GameObject& aFirst,
        GameObject& aSecond,
        const Vector3f& aNormal,
        float aPenetration,
        const std::unordered_map<std::uint64_t, CollisionPairState>& somePreviousPairs,
        std::unordered_map<std::uint64_t, CollisionPairState>& someCurrentPairs,
        std::vector<CollisionContact>& outContacts)
    {
        const std::uint64_t pairKey = MakeCollisionPairKey(aFirst, aSecond);
        if (someCurrentPairs.find(pairKey) != someCurrentPairs.end())
        {
            return nullptr;
        }

        CollisionPairState pairState;
        pairState.firstId = aFirst.GetCollisionId();
        pairState.secondId = aSecond.GetCollisionId();
        pairState.first = &aFirst;
        pairState.second = &aSecond;
        someCurrentPairs.emplace(pairKey, pairState);

        CollisionContact contact;
        contact.first = &aFirst;
        contact.second = &aSecond;
        contact.normal = aNormal;
        contact.penetration = aPenetration;
        contact.phase = somePreviousPairs.find(pairKey) != somePreviousPairs.end()
            ? CollisionPhase::Stay
            : CollisionPhase::Enter;
        outContacts.push_back(contact);
        return &outContacts.back();
    }

    std::size_t AppendExitContacts(
        const std::unordered_map<std::uint64_t, CollisionPairState>& somePreviousPairs,
        const std::unordered_map<std::uint64_t, CollisionPairState>& someCurrentPairs,
        const std::unordered_map<std::uint64_t, GameObject*>& someLiveColliderObjectsById,
        std::vector<CollisionContact>& outContacts)
    {
        const std::size_t firstExitIndex = outContacts.size();
        for (const auto& [pairKey, pair] : somePreviousPairs)
        {
            if (someCurrentPairs.find(pairKey) != someCurrentPairs.end())
            {
                continue;
            }

            auto firstIt = someLiveColliderObjectsById.find(pair.firstId);
            auto secondIt = someLiveColliderObjectsById.find(pair.secondId);
            if (firstIt == someLiveColliderObjectsById.end() || secondIt == someLiveColliderObjectsById.end())
            {
                continue;
            }

            CollisionContact contact;
            contact.first = firstIt->second;
            contact.second = secondIt->second;
            contact.phase = CollisionPhase::Exit;
            outContacts.push_back(contact);
        }

        return firstExitIndex;
    }
}
