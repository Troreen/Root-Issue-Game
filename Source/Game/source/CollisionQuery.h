#pragma once

#include "CollisionShapeType.h"

#include <CommonUtilities/AABB3D.hpp>
#include <CommonUtilities/Vector3.hpp>

class GameObject;

namespace CollisionQuery
{
    using Vector3f = CommonUtilities::Vector3<float>;

    struct Shape
    {
        CollisionShapeType type = CollisionShapeType::Box;
        CommonUtilities::AABB3D<float> bounds;
        Vector3f center = Vector3f::Zero;
        Vector3f segmentA = Vector3f::Zero;
        Vector3f segmentB = Vector3f::Zero;
        Vector3f axes[3] = { Vector3f::UnitX, Vector3f::UnitY, Vector3f::UnitZ };
        Vector3f halfExtents = Vector3f::Zero;
        float radius = 0.0f;
        bool isValid = false;
    };

    bool HasRuntimeCollider(const GameObject& anObject);
    void RefreshRuntimeCollider(GameObject& anObject);

    Shape MakeBoxShape(const Vector3f& aCenter, const Vector3f& aSize);
    Shape GetShape(const GameObject& anObject);
    bool TryComputeSeparation(
        const Shape& aFirst,
        const Shape& aSecond,
        Vector3f& outSeparation,
        Vector3f& outNormal,
        float& outPenetration);
}
