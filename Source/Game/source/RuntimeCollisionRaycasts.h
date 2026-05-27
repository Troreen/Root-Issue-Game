#pragma once

#include "CollisionTypes.h"
#include "RuntimeCollisionTypes.h"

namespace RuntimeCollision
{
    bool TryRaycastShape(
        const Vector3f& anOrigin,
        const Vector3f& aDirection,
        float aMaxDistance,
        const CollisionShape& aShape,
        float aRadiusPadding,
        CollisionRaycastHit& outHit);
}
