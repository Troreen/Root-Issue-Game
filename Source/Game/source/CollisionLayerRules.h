#pragma once

#include "CollisionTypes.h"
#include "ObjectLayer.h"

#include <array>
#include <cstddef>

constexpr std::size_t kObjectLayerCount = static_cast<std::size_t>(ObjectLayer::Count);

class CollisionLayerRuleTable
{
public:
    CollisionLayerRuleTable()
    {
        myRules.fill(CollisionRule::Ignore);
    }

    void Set(ObjectLayer aLeft, ObjectLayer aRight, CollisionRule aRule)
    {
        myRules[Index(aLeft, aRight)] = aRule;
    }

    void SetSymmetric(ObjectLayer aLeft, ObjectLayer aRight, CollisionRule aRule)
    {
        Set(aLeft, aRight, aRule);
        Set(aRight, aLeft, aRule);
    }

    CollisionRule Get(ObjectLayer aLeft, ObjectLayer aRight) const
    {
        return myRules[Index(aLeft, aRight)];
    }

private:
    static constexpr std::size_t Index(ObjectLayer aLeft, ObjectLayer aRight)
    {
        return static_cast<std::size_t>(aLeft) * kObjectLayerCount + static_cast<std::size_t>(aRight);
    }

    std::array<CollisionRule, kObjectLayerCount * kObjectLayerCount> myRules;
};
