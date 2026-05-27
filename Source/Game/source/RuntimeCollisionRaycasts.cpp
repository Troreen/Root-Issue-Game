#include "RuntimeCollisionRaycasts.h"

#include <algorithm>
#include <cmath>

namespace RuntimeCollision
{
    CommonUtilities::AABB3D<float> ExpandAabb(const CommonUtilities::AABB3D<float>& anAabb, const float aPadding)
    {
        const Vector3f padding(aPadding, aPadding, aPadding);
        return CommonUtilities::AABB3D<float>(anAabb.GetMin() - padding, anAabb.GetMax() + padding);
    }

    bool TryRaycastAabb(
        const Vector3f& anOrigin,
        const Vector3f& aDirection,
        const float aMaxDistance,
        const CommonUtilities::AABB3D<float>& anAabb,
        float& outDistance,
        Vector3f& outNormal)
    {
        if (aMaxDistance <= 0.0f || aDirection.LengthSqr() <= kCollisionEpsilon)
        {
            return false;
        }

        const float origin[3] = { anOrigin.x, anOrigin.y, anOrigin.z };
        const float direction[3] = { aDirection.x, aDirection.y, aDirection.z };
        const float minimum[3] = { anAabb.GetMin().x, anAabb.GetMin().y, anAabb.GetMin().z };
        const float maximum[3] = { anAabb.GetMax().x, anAabb.GetMax().y, anAabb.GetMax().z };
        const Vector3f axisNormals[3] = { Vector3f::UnitX, Vector3f::UnitY, Vector3f::UnitZ };

        float entryDistance = 0.0f;
        float exitDistance = aMaxDistance;
        Vector3f entryNormal = Vector3f::Zero;

        for (int axis = 0; axis < 3; ++axis)
        {
            if (std::abs(direction[axis]) <= kCollisionEpsilon)
            {
                if (origin[axis] < minimum[axis] || origin[axis] > maximum[axis])
                {
                    return false;
                }

                continue;
            }

            const float inverseDirection = 1.0f / direction[axis];
            float first = (minimum[axis] - origin[axis]) * inverseDirection;
            float second = (maximum[axis] - origin[axis]) * inverseDirection;
            Vector3f normal = -axisNormals[axis];
            if (first > second)
            {
                std::swap(first, second);
                normal = axisNormals[axis];
            }

            if (first > entryDistance)
            {
                entryDistance = first;
                entryNormal = normal;
            }

            exitDistance = (std::min)(exitDistance, second);
            if (entryDistance > exitDistance)
            {
                return false;
            }
        }

        outDistance = entryDistance;
        outNormal = entryNormal.LengthSqr() > kCollisionEpsilon
            ? entryNormal
            : -aDirection.GetNormalized();
        return outDistance <= aMaxDistance;
    }

    bool TryRaycastShape(
        const Vector3f& anOrigin,
        const Vector3f& aDirection,
        const float aMaxDistance,
        const CollisionShape& aShape,
        const float aRadiusPadding,
        CollisionRaycastHit& outHit)
    {
        if (!aShape.isValid)
        {
            return false;
        }

        float distance = 0.0f;
        Vector3f normal = Vector3f::Zero;
        if (!TryRaycastAabb(
            anOrigin,
            aDirection,
            aMaxDistance,
            ExpandAabb(aShape.bounds, aRadiusPadding),
            distance,
            normal))
        {
            return false;
        }

        outHit.distance = distance;
        outHit.point = anOrigin + aDirection.GetNormalized() * distance;
        outHit.normal = normal;
        return true;
    }

    float GetSweepRadius(const CollisionShape& aShape)
    {
        switch (aShape.type)
        {
        case CollisionShapeType::Sphere:
        case CollisionShapeType::Capsule:
            return aShape.radius;
        case CollisionShapeType::Box:
        case CollisionShapeType::Obb:
        default:
            return (std::max)(aShape.halfExtents.x, (std::max)(aShape.halfExtents.y, aShape.halfExtents.z));
        }
    }
}
