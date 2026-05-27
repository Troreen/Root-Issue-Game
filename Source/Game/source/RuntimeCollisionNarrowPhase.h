#pragma once

#include "RuntimeCollisionTypes.h"

namespace RuntimeCollision
{
    bool TryComputeShapeSeparation(
        const CollisionShape& aFirst,
        const CollisionShape& aSecond,
        Vector3f& outSeparation,
        Vector3f& outNormal,
        float& outPenetration);

    bool Has3DOverlap(
        const CommonUtilities::AABB3D<float>& aFirstAabb,
        const CommonUtilities::AABB3D<float>& aSecondAabb);

    Vector3f GetOverlapDepths(
        const CommonUtilities::AABB3D<float>& aFirstAabb,
        const CommonUtilities::AABB3D<float>& aSecondAabb);

    float GetSweepRadius(const CollisionShape& aShape);
}
