#pragma once

#include "CollisionTypes.h"
#include "CollisionShapeType.h"

#include <CommonUtilities/AABB3D.hpp>
#include <CommonUtilities/Vector3.hpp>

#include <cstdint>

class GameObject;

namespace RuntimeCollision
{
    using Vector3f = CommonUtilities::Vector3<float>;

    constexpr int kMaxCollisionIterations = 4;
    constexpr float kCollisionEpsilon = 0.0001f;

    struct CollisionShape
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

    struct CollisionPairState
    {
        std::uint64_t firstId = 0;
        std::uint64_t secondId = 0;
        GameObject* first = nullptr;
        GameObject* second = nullptr;
    };
}
