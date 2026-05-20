#include "RuntimeCollisionSystem.h"

#include "BoxColliderComponent.h"
#include "CapsuleColliderComponent.h"
#include "CollisionLayerRules.h"
#include "CollisionShapeType.h"
#include "DebugSettings.h"
#include "GameObject.h"
#include "ObbColliderComponent.h"
#include "ObjectLayer.h"
#include "SphereColliderComponent.h"

#include <CommonUtilities/Vector3.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

using Vector3f = CommonUtilities::Vector3<float>;

namespace
{
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

    bool Has3DOverlap(
        const CommonUtilities::AABB3D<float>& aFirstAabb,
        const CommonUtilities::AABB3D<float>& aSecondAabb);

    const char* ToLayerName(ObjectLayer aLayer)
    {
        switch (aLayer)
        {
        case ObjectLayer::WorldStatic:
            return "WorldStatic";
        case ObjectLayer::Player:
            return "Player";
        case ObjectLayer::Enemy:
            return "Enemy";
        case ObjectLayer::Projectile:
            return "Projectile";
        case ObjectLayer::Trigger:
            return "Trigger";
        case ObjectLayer::Pickup:
            return "Pickup";
        case ObjectLayer::NPC:
            return "NPC";
        case ObjectLayer::Count:
        default:
            return "Unknown";
        }
    }

    bool HasRuntimeCollider(const GameObject& anObject)
    {
        return anObject.GetComponent<BoxColliderComponent>() != nullptr ||
            anObject.GetComponent<SphereColliderComponent>() != nullptr ||
            anObject.GetComponent<CapsuleColliderComponent>() != nullptr ||
            anObject.GetComponent<ObbColliderComponent>() != nullptr;
    }

    bool HasTriggerCollider(const GameObject& anObject)
    {
        if (const auto* box = anObject.GetComponent<BoxColliderComponent>())
        {
            return box->IsTrigger();
        }

        if (const auto* sphere = anObject.GetComponent<SphereColliderComponent>())
        {
            return sphere->IsTrigger();
        }

        if (const auto* capsule = anObject.GetComponent<CapsuleColliderComponent>())
        {
            return capsule->IsTrigger();
        }

        if (const auto* obb = anObject.GetComponent<ObbColliderComponent>())
        {
            return obb->IsTrigger();
        }

        return false;
    }

    void RefreshRuntimeCollider(GameObject& anObject)
    {
        if (auto* box = anObject.GetComponent<BoxColliderComponent>())
        {
            box->Update(0.0f);
        }
        else if (auto* sphere = anObject.GetComponent<SphereColliderComponent>())
        {
            sphere->Update(0.0f);
        }
        else if (auto* capsule = anObject.GetComponent<CapsuleColliderComponent>())
        {
            capsule->Update(0.0f);
        }
        else if (auto* obb = anObject.GetComponent<ObbColliderComponent>())
        {
            obb->Update(0.0f);
        }
    }

    std::uint64_t MakeCollisionPairKey(const GameObject& aFirst, const GameObject& aSecond)
    {
        const std::uint64_t low = (std::min)(aFirst.GetCollisionId(), aSecond.GetCollisionId());
        const std::uint64_t high = (std::max)(aFirst.GetCollisionId(), aSecond.GetCollisionId());
        return low ^ (high + 0x9e3779b97f4a7c15ULL + (low << 6) + (low >> 2));
    }

    float Clamp(float aValue, float aMin, float aMax)
    {
        return (std::max)(aMin, (std::min)(aValue, aMax));
    }

    Vector3f ClosestPointOnAabb(const Vector3f& aPoint, const CommonUtilities::AABB3D<float>& anAabb)
    {
        return Vector3f(
            Clamp(aPoint.x, anAabb.GetMin().x, anAabb.GetMax().x),
            Clamp(aPoint.y, anAabb.GetMin().y, anAabb.GetMax().y),
            Clamp(aPoint.z, anAabb.GetMin().z, anAabb.GetMax().z));
    }

    Vector3f ClosestPointOnSegment(const Vector3f& aPoint, const Vector3f& aStart, const Vector3f& anEnd)
    {
        const Vector3f segment = anEnd - aStart;
        const float lengthSquared = segment.LengthSqr();
        if (lengthSquared <= kCollisionEpsilon)
        {
            return aStart;
        }

        const float t = Clamp((aPoint - aStart).Dot(segment) / lengthSquared, 0.0f, 1.0f);
        return aStart + segment * t;
    }

    bool TryAabbAabbSeparation(
        const CommonUtilities::AABB3D<float>& aFirstAabb,
        const CommonUtilities::AABB3D<float>& aSecondAabb,
        Vector3f& outSeparation,
        Vector3f& outNormal,
        float& outPenetration)
    {
        const float overlapX =
            (std::min)(aFirstAabb.GetMax().x, aSecondAabb.GetMax().x) -
            (std::max)(aFirstAabb.GetMin().x, aSecondAabb.GetMin().x);
        const float overlapZ =
            (std::min)(aFirstAabb.GetMax().z, aSecondAabb.GetMax().z) -
            (std::max)(aFirstAabb.GetMin().z, aSecondAabb.GetMin().z);
        const float overlapY =
            (std::min)(aFirstAabb.GetMax().y, aSecondAabb.GetMax().y) -
            (std::max)(aFirstAabb.GetMin().y, aSecondAabb.GetMin().y);

        if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f)
        {
            return false;
        }

        const Vector3f firstCenter = (aFirstAabb.GetMin() + aFirstAabb.GetMax()) * 0.5f;
        const Vector3f secondCenter = (aSecondAabb.GetMin() + aSecondAabb.GetMax()) * 0.5f;

        outSeparation = Vector3f::Zero;
        outNormal = Vector3f::Zero;

        if (overlapX <= overlapY && overlapX <= overlapZ)
        {
            outPenetration = overlapX;
            const float direction = firstCenter.x < secondCenter.x ? -1.0f : 1.0f;
            outSeparation.x = outPenetration * direction;
            outNormal.x = direction;
        }
        else if (overlapY <= overlapZ)
        {
            outPenetration = overlapY;
            const float direction = firstCenter.y < secondCenter.y ? -1.0f : 1.0f;
            outSeparation.y = outPenetration * direction;
            outNormal.y = direction;
        }
        else
        {
            outPenetration = overlapZ;
            const float direction = firstCenter.z < secondCenter.z ? -1.0f : 1.0f;
            outSeparation.z = outPenetration * direction;
            outNormal.z = direction;
        }

        return true;
    }

    bool TrySphereSphereSeparation(
        const Vector3f& aFirstCenter,
        float aFirstRadius,
        const Vector3f& aSecondCenter,
        float aSecondRadius,
        Vector3f& outSeparation,
        Vector3f& outNormal,
        float& outPenetration)
    {
        Vector3f delta = aFirstCenter - aSecondCenter;
        float distance = delta.Length();
        const float radiusSum = aFirstRadius + aSecondRadius;
        if (distance >= radiusSum)
        {
            return false;
        }

        if (distance <= kCollisionEpsilon)
        {
            outNormal = Vector3f::UnitX;
            outPenetration = radiusSum;
        }
        else
        {
            outNormal = delta / distance;
            outPenetration = radiusSum - distance;
        }

        outSeparation = outNormal * outPenetration;
        return true;
    }

    bool TrySphereAabbSeparation(
        const Vector3f& aSphereCenter,
        float aSphereRadius,
        const CommonUtilities::AABB3D<float>& anAabb,
        Vector3f& outSeparation,
        Vector3f& outNormal,
        float& outPenetration)
    {
        const Vector3f closest = ClosestPointOnAabb(aSphereCenter, anAabb);
        Vector3f delta = aSphereCenter - closest;
        const float distance = delta.Length();
        if (distance > kCollisionEpsilon)
        {
            if (distance >= aSphereRadius)
            {
                return false;
            }

            outNormal = delta / distance;
            outPenetration = aSphereRadius - distance;
            outSeparation = outNormal * outPenetration;
            return true;
        }

        if (aSphereCenter.x < anAabb.GetMin().x || aSphereCenter.x > anAabb.GetMax().x ||
            aSphereCenter.y < anAabb.GetMin().y || aSphereCenter.y > anAabb.GetMax().y ||
            aSphereCenter.z < anAabb.GetMin().z || aSphereCenter.z > anAabb.GetMax().z)
        {
            return false;
        }

        const float toMinX = aSphereCenter.x - anAabb.GetMin().x;
        const float toMaxX = anAabb.GetMax().x - aSphereCenter.x;
        const float toMinY = aSphereCenter.y - anAabb.GetMin().y;
        const float toMaxY = anAabb.GetMax().y - aSphereCenter.y;
        const float toMinZ = aSphereCenter.z - anAabb.GetMin().z;
        const float toMaxZ = anAabb.GetMax().z - aSphereCenter.z;

        float closestFaceDistance = toMinX;
        outNormal = Vector3f(-1.0f, 0.0f, 0.0f);

        auto chooseFace = [&](float aDistance, const Vector3f& aNormal)
        {
            if (aDistance < closestFaceDistance)
            {
                closestFaceDistance = aDistance;
                outNormal = aNormal;
            }
        };

        chooseFace(toMaxX, Vector3f(1.0f, 0.0f, 0.0f));
        chooseFace(toMinY, Vector3f(0.0f, -1.0f, 0.0f));
        chooseFace(toMaxY, Vector3f(0.0f, 1.0f, 0.0f));
        chooseFace(toMinZ, Vector3f(0.0f, 0.0f, -1.0f));
        chooseFace(toMaxZ, Vector3f(0.0f, 0.0f, 1.0f));

        outPenetration = aSphereRadius + closestFaceDistance;
        outSeparation = outNormal * outPenetration;
        return true;
    }

    bool TryCapsuleAabbSeparation(
        const Vector3f& aCapsuleStart,
        const Vector3f& aCapsuleEnd,
        float aCapsuleRadius,
        const CommonUtilities::AABB3D<float>& anAabb,
        Vector3f& outSeparation,
        Vector3f& outNormal,
        float& outPenetration)
    {
        const float segmentMinY = (std::min)(aCapsuleStart.y, aCapsuleEnd.y);
        const float segmentMaxY = (std::max)(aCapsuleStart.y, aCapsuleEnd.y);
        float segmentY = 0.0f;
        float boxY = 0.0f;

        if (segmentMaxY < anAabb.GetMin().y)
        {
            segmentY = segmentMaxY;
            boxY = anAabb.GetMin().y;
        }
        else if (segmentMinY > anAabb.GetMax().y)
        {
            segmentY = segmentMinY;
            boxY = anAabb.GetMax().y;
        }
        else
        {
            segmentY = Clamp((std::max)(segmentMinY, anAabb.GetMin().y), segmentMinY, segmentMaxY);
            boxY = segmentY;
        }

        const Vector3f segmentPoint(aCapsuleStart.x, segmentY, aCapsuleStart.z);
        const Vector3f boxPoint(
            Clamp(aCapsuleStart.x, anAabb.GetMin().x, anAabb.GetMax().x),
            boxY,
            Clamp(aCapsuleStart.z, anAabb.GetMin().z, anAabb.GetMax().z));

        Vector3f delta = segmentPoint - boxPoint;
        const float distance = delta.Length();
        if (distance > kCollisionEpsilon)
        {
            if (distance >= aCapsuleRadius)
            {
                return false;
            }

            outNormal = delta / distance;
            outPenetration = aCapsuleRadius - distance;
            outSeparation = outNormal * outPenetration;
            return true;
        }

        const CommonUtilities::AABB3D<float> capsuleBounds(
            Vector3f(
                (std::min)(aCapsuleStart.x, aCapsuleEnd.x) - aCapsuleRadius,
                segmentMinY - aCapsuleRadius,
                (std::min)(aCapsuleStart.z, aCapsuleEnd.z) - aCapsuleRadius),
            Vector3f(
                (std::max)(aCapsuleStart.x, aCapsuleEnd.x) + aCapsuleRadius,
                segmentMaxY + aCapsuleRadius,
                (std::max)(aCapsuleStart.z, aCapsuleEnd.z) + aCapsuleRadius));
        return TryAabbAabbSeparation(capsuleBounds, anAabb, outSeparation, outNormal, outPenetration);
    }

    void ClosestPointsBetweenSegments(
        const Vector3f& aStart,
        const Vector3f& anEnd,
        const Vector3f& anotherStart,
        const Vector3f& anotherEnd,
        Vector3f& outFirst,
        Vector3f& outSecond)
    {
        const Vector3f firstDirection = anEnd - aStart;
        const Vector3f secondDirection = anotherEnd - anotherStart;
        const Vector3f startDelta = aStart - anotherStart;
        const float a = firstDirection.Dot(firstDirection);
        const float e = secondDirection.Dot(secondDirection);
        const float f = secondDirection.Dot(startDelta);

        float s = 0.0f;
        float t = 0.0f;
        if (a <= kCollisionEpsilon && e <= kCollisionEpsilon)
        {
            outFirst = aStart;
            outSecond = anotherStart;
            return;
        }

        if (a <= kCollisionEpsilon)
        {
            t = Clamp(f / e, 0.0f, 1.0f);
        }
        else
        {
            const float c = firstDirection.Dot(startDelta);
            if (e <= kCollisionEpsilon)
            {
                s = Clamp(-c / a, 0.0f, 1.0f);
            }
            else
            {
                const float b = firstDirection.Dot(secondDirection);
                const float denominator = a * e - b * b;
                if (denominator != 0.0f)
                {
                    s = Clamp((b * f - c * e) / denominator, 0.0f, 1.0f);
                }

                t = (b * s + f) / e;
                if (t < 0.0f)
                {
                    t = 0.0f;
                    s = Clamp(-c / a, 0.0f, 1.0f);
                }
                else if (t > 1.0f)
                {
                    t = 1.0f;
                    s = Clamp((b - c) / a, 0.0f, 1.0f);
                }
            }
        }

        outFirst = aStart + firstDirection * s;
        outSecond = anotherStart + secondDirection * t;
    }

    bool TryCapsuleSphereSeparation(
        const Vector3f& aCapsuleStart,
        const Vector3f& aCapsuleEnd,
        float aCapsuleRadius,
        const Vector3f& aSphereCenter,
        float aSphereRadius,
        Vector3f& outSeparation,
        Vector3f& outNormal,
        float& outPenetration)
    {
        const Vector3f capsulePoint = ClosestPointOnSegment(aSphereCenter, aCapsuleStart, aCapsuleEnd);
        return TrySphereSphereSeparation(capsulePoint, aCapsuleRadius, aSphereCenter, aSphereRadius, outSeparation, outNormal, outPenetration);
    }

    bool TryCapsuleCapsuleSeparation(
        const Vector3f& aFirstStart,
        const Vector3f& aFirstEnd,
        float aFirstRadius,
        const Vector3f& aSecondStart,
        const Vector3f& aSecondEnd,
        float aSecondRadius,
        Vector3f& outSeparation,
        Vector3f& outNormal,
        float& outPenetration)
    {
        Vector3f firstPoint;
        Vector3f secondPoint;
        ClosestPointsBetweenSegments(aFirstStart, aFirstEnd, aSecondStart, aSecondEnd, firstPoint, secondPoint);
        return TrySphereSphereSeparation(firstPoint, aFirstRadius, secondPoint, aSecondRadius, outSeparation, outNormal, outPenetration);
    }

    CollisionShape MakeObbFromAabb(const CommonUtilities::AABB3D<float>& anAabb)
    {
        CollisionShape shape;
        shape.type = CollisionShapeType::Obb;
        shape.bounds = anAabb;
        shape.center = (anAabb.GetMin() + anAabb.GetMax()) * 0.5f;
        shape.halfExtents = (anAabb.GetMax() - anAabb.GetMin()) * 0.5f;
        shape.axes[0] = Vector3f::UnitX;
        shape.axes[1] = Vector3f::UnitY;
        shape.axes[2] = Vector3f::UnitZ;
        shape.isValid = true;
        return shape;
    }

    Vector3f WorldPointToObbLocal(const Vector3f& aPoint, const CollisionShape& anObb)
    {
        const Vector3f delta = aPoint - anObb.center;
        return Vector3f(
            delta.Dot(anObb.axes[0]),
            delta.Dot(anObb.axes[1]),
            delta.Dot(anObb.axes[2]));
    }

    Vector3f ObbLocalVectorToWorld(const Vector3f& aVector, const CollisionShape& anObb)
    {
        return anObb.axes[0] * aVector.x + anObb.axes[1] * aVector.y + anObb.axes[2] * aVector.z;
    }

    Vector3f ClosestPointOnObb(const Vector3f& aPoint, const CollisionShape& anObb)
    {
        const Vector3f local = WorldPointToObbLocal(aPoint, anObb);
        return anObb.center + ObbLocalVectorToWorld(Vector3f(
            Clamp(local.x, -anObb.halfExtents.x, anObb.halfExtents.x),
            Clamp(local.y, -anObb.halfExtents.y, anObb.halfExtents.y),
            Clamp(local.z, -anObb.halfExtents.z, anObb.halfExtents.z)), anObb);
    }

    float ProjectObbRadius(const CollisionShape& anObb, const Vector3f& anAxis)
    {
        return anObb.halfExtents.x * std::abs(anAxis.Dot(anObb.axes[0])) +
            anObb.halfExtents.y * std::abs(anAxis.Dot(anObb.axes[1])) +
            anObb.halfExtents.z * std::abs(anAxis.Dot(anObb.axes[2]));
    }

    bool TryObbObbSeparation(
        const CollisionShape& aFirst,
        const CollisionShape& aSecond,
        Vector3f& outSeparation,
        Vector3f& outNormal,
        float& outPenetration)
    {
        Vector3f axesToTest[15];
        int axisCount = 0;
        for (int i = 0; i < 3; ++i)
        {
            axesToTest[axisCount++] = aFirst.axes[i];
            axesToTest[axisCount++] = aSecond.axes[i];
        }

        for (int firstAxis = 0; firstAxis < 3; ++firstAxis)
        {
            for (int secondAxis = 0; secondAxis < 3; ++secondAxis)
            {
                axesToTest[axisCount++] = aFirst.axes[firstAxis].Cross(aSecond.axes[secondAxis]);
            }
        }

        float smallestOverlap = (std::numeric_limits<float>::max)();
        Vector3f bestAxis = Vector3f::UnitX;
        const Vector3f centerDelta = aFirst.center - aSecond.center;

        for (int axisIndex = 0; axisIndex < axisCount; ++axisIndex)
        {
            Vector3f axis = axesToTest[axisIndex];
            const float axisLength = axis.Length();
            if (axisLength <= kCollisionEpsilon)
            {
                continue;
            }
            axis /= axisLength;

            const float firstRadius = ProjectObbRadius(aFirst, axis);
            const float secondRadius = ProjectObbRadius(aSecond, axis);
            const float centerDistance = std::abs(centerDelta.Dot(axis));
            const float overlap = firstRadius + secondRadius - centerDistance;
            if (overlap <= 0.0f)
            {
                return false;
            }

            if (overlap < smallestOverlap)
            {
                smallestOverlap = overlap;
                bestAxis = centerDelta.Dot(axis) < 0.0f ? -axis : axis;
            }
        }

        outNormal = bestAxis;
        outPenetration = smallestOverlap;
        outSeparation = outNormal * outPenetration;
        return true;
    }

    bool TrySphereObbSeparation(
        const Vector3f& aSphereCenter,
        float aSphereRadius,
        const CollisionShape& anObb,
        Vector3f& outSeparation,
        Vector3f& outNormal,
        float& outPenetration)
    {
        const Vector3f closest = ClosestPointOnObb(aSphereCenter, anObb);
        Vector3f delta = aSphereCenter - closest;
        const float distance = delta.Length();
        if (distance > kCollisionEpsilon)
        {
            if (distance >= aSphereRadius)
            {
                return false;
            }

            outNormal = delta / distance;
            outPenetration = aSphereRadius - distance;
            outSeparation = outNormal * outPenetration;
            return true;
        }

        const Vector3f local = WorldPointToObbLocal(aSphereCenter, anObb);
        if (std::abs(local.x) > anObb.halfExtents.x ||
            std::abs(local.y) > anObb.halfExtents.y ||
            std::abs(local.z) > anObb.halfExtents.z)
        {
            return false;
        }

        float closestFaceDistance = anObb.halfExtents.x - std::abs(local.x);
        outNormal = anObb.axes[0] * (local.x < 0.0f ? -1.0f : 1.0f);

        auto chooseFace = [&](float aDistance, const Vector3f& aNormal)
        {
            if (aDistance < closestFaceDistance)
            {
                closestFaceDistance = aDistance;
                outNormal = aNormal;
            }
        };

        chooseFace(anObb.halfExtents.y - std::abs(local.y), anObb.axes[1] * (local.y < 0.0f ? -1.0f : 1.0f));
        chooseFace(anObb.halfExtents.z - std::abs(local.z), anObb.axes[2] * (local.z < 0.0f ? -1.0f : 1.0f));

        outPenetration = aSphereRadius + closestFaceDistance;
        outSeparation = outNormal * outPenetration;
        return true;
    }

    bool TryCapsuleObbSeparation(
        const Vector3f& aCapsuleStart,
        const Vector3f& aCapsuleEnd,
        float aCapsuleRadius,
        const CollisionShape& anObb,
        Vector3f& outSeparation,
        Vector3f& outNormal,
        float& outPenetration)
    {
        const Vector3f localStart = WorldPointToObbLocal(aCapsuleStart, anObb);
        const Vector3f localEnd = WorldPointToObbLocal(aCapsuleEnd, anObb);
        const CommonUtilities::AABB3D<float> localAabb(-anObb.halfExtents, anObb.halfExtents);
        Vector3f localSeparation;
        Vector3f localNormal;
        if (!TryCapsuleAabbSeparation(localStart, localEnd, aCapsuleRadius, localAabb, localSeparation, localNormal, outPenetration))
        {
            return false;
        }

        outSeparation = ObbLocalVectorToWorld(localSeparation, anObb);
        outNormal = ObbLocalVectorToWorld(localNormal, anObb).GetNormalized();
        return true;
    }

    CollisionShape GetCollisionShape(const GameObject& anObject)
    {
        CollisionShape shape;

        if (const auto* box = anObject.GetComponent<BoxColliderComponent>())
        {
            shape.type = CollisionShapeType::Box;
            shape.bounds = box->GetAABB();
            shape.center = (shape.bounds.GetMin() + shape.bounds.GetMax()) * 0.5f;
            shape.halfExtents = (shape.bounds.GetMax() - shape.bounds.GetMin()) * 0.5f;
            shape.axes[0] = Vector3f::UnitX;
            shape.axes[1] = Vector3f::UnitY;
            shape.axes[2] = Vector3f::UnitZ;
            shape.isValid = true;
            return shape;
        }

        if (const auto* sphere = anObject.GetComponent<SphereColliderComponent>())
        {
            shape.type = CollisionShapeType::Sphere;
            shape.center = sphere->GetSphere().GetCenter();
            shape.radius = sphere->GetSphere().GetRadius();
            shape.bounds = sphere->GetAABB();
            shape.isValid = true;
            return shape;
        }

        if (const auto* capsule = anObject.GetComponent<CapsuleColliderComponent>())
        {
            shape.type = CollisionShapeType::Capsule;
            shape.segmentA = capsule->GetBottomCenter();
            shape.segmentB = capsule->GetTopCenter();
            shape.radius = capsule->GetRadius();
            shape.bounds = capsule->GetAABB();
            shape.center = (shape.segmentA + shape.segmentB) * 0.5f;
            shape.isValid = true;
            return shape;
        }

        if (const auto* obb = anObject.GetComponent<ObbColliderComponent>())
        {
            shape.type = CollisionShapeType::Obb;
            shape.bounds = obb->GetAABB();
            shape.center = obb->GetCenter();
            shape.halfExtents = obb->GetHalfExtents();
            const Vector3f* axes = obb->GetAxes();
            shape.axes[0] = axes[0];
            shape.axes[1] = axes[1];
            shape.axes[2] = axes[2];
            shape.isValid = true;
            return shape;
        }

        return shape;
    }

    bool TryComputeShapeSeparation(
        const CollisionShape& aFirst,
        const CollisionShape& aSecond,
        Vector3f& outSeparation,
        Vector3f& outNormal,
        float& outPenetration)
    {
        if (!aFirst.isValid || !aSecond.isValid || !Has3DOverlap(aFirst.bounds, aSecond.bounds))
        {
            return false;
        }

        if (aFirst.type == CollisionShapeType::Box && aSecond.type == CollisionShapeType::Box)
        {
            return TryAabbAabbSeparation(aFirst.bounds, aSecond.bounds, outSeparation, outNormal, outPenetration);
        }

        if (aFirst.type == CollisionShapeType::Sphere && aSecond.type == CollisionShapeType::Sphere)
        {
            return TrySphereSphereSeparation(aFirst.center, aFirst.radius, aSecond.center, aSecond.radius, outSeparation, outNormal, outPenetration);
        }

        if (aFirst.type == CollisionShapeType::Sphere && aSecond.type == CollisionShapeType::Box)
        {
            return TrySphereAabbSeparation(aFirst.center, aFirst.radius, aSecond.bounds, outSeparation, outNormal, outPenetration);
        }

        if (aFirst.type == CollisionShapeType::Box && aSecond.type == CollisionShapeType::Sphere)
        {
            const bool hit = TrySphereAabbSeparation(aSecond.center, aSecond.radius, aFirst.bounds, outSeparation, outNormal, outPenetration);
            if (hit)
            {
                outSeparation = -outSeparation;
                outNormal = -outNormal;
            }
            return hit;
        }

        if (aFirst.type == CollisionShapeType::Capsule && aSecond.type == CollisionShapeType::Box)
        {
            return TryCapsuleAabbSeparation(aFirst.segmentA, aFirst.segmentB, aFirst.radius, aSecond.bounds, outSeparation, outNormal, outPenetration);
        }

        if (aFirst.type == CollisionShapeType::Box && aSecond.type == CollisionShapeType::Capsule)
        {
            const bool hit = TryCapsuleAabbSeparation(aSecond.segmentA, aSecond.segmentB, aSecond.radius, aFirst.bounds, outSeparation, outNormal, outPenetration);
            if (hit)
            {
                outSeparation = -outSeparation;
                outNormal = -outNormal;
            }
            return hit;
        }

        if (aFirst.type == CollisionShapeType::Capsule && aSecond.type == CollisionShapeType::Sphere)
        {
            return TryCapsuleSphereSeparation(aFirst.segmentA, aFirst.segmentB, aFirst.radius, aSecond.center, aSecond.radius, outSeparation, outNormal, outPenetration);
        }

        if (aFirst.type == CollisionShapeType::Sphere && aSecond.type == CollisionShapeType::Capsule)
        {
            const bool hit = TryCapsuleSphereSeparation(aSecond.segmentA, aSecond.segmentB, aSecond.radius, aFirst.center, aFirst.radius, outSeparation, outNormal, outPenetration);
            if (hit)
            {
                outSeparation = -outSeparation;
                outNormal = -outNormal;
            }
            return hit;
        }

        if (aFirst.type == CollisionShapeType::Capsule && aSecond.type == CollisionShapeType::Capsule)
        {
            return TryCapsuleCapsuleSeparation(
                aFirst.segmentA, aFirst.segmentB, aFirst.radius,
                aSecond.segmentA, aSecond.segmentB, aSecond.radius,
                outSeparation, outNormal, outPenetration);
        }

        if (aFirst.type == CollisionShapeType::Obb && aSecond.type == CollisionShapeType::Obb)
        {
            return TryObbObbSeparation(aFirst, aSecond, outSeparation, outNormal, outPenetration);
        }

        if (aFirst.type == CollisionShapeType::Obb && aSecond.type == CollisionShapeType::Box)
        {
            return TryObbObbSeparation(aFirst, MakeObbFromAabb(aSecond.bounds), outSeparation, outNormal, outPenetration);
        }

        if (aFirst.type == CollisionShapeType::Box && aSecond.type == CollisionShapeType::Obb)
        {
            const bool hit = TryObbObbSeparation(aSecond, MakeObbFromAabb(aFirst.bounds), outSeparation, outNormal, outPenetration);
            if (hit)
            {
                outSeparation = -outSeparation;
                outNormal = -outNormal;
            }
            return hit;
        }

        if (aFirst.type == CollisionShapeType::Sphere && aSecond.type == CollisionShapeType::Obb)
        {
            return TrySphereObbSeparation(aFirst.center, aFirst.radius, aSecond, outSeparation, outNormal, outPenetration);
        }

        if (aFirst.type == CollisionShapeType::Obb && aSecond.type == CollisionShapeType::Sphere)
        {
            const bool hit = TrySphereObbSeparation(aSecond.center, aSecond.radius, aFirst, outSeparation, outNormal, outPenetration);
            if (hit)
            {
                outSeparation = -outSeparation;
                outNormal = -outNormal;
            }
            return hit;
        }

        if (aFirst.type == CollisionShapeType::Capsule && aSecond.type == CollisionShapeType::Obb)
        {
            return TryCapsuleObbSeparation(aFirst.segmentA, aFirst.segmentB, aFirst.radius, aSecond, outSeparation, outNormal, outPenetration);
        }

        if (aFirst.type == CollisionShapeType::Obb && aSecond.type == CollisionShapeType::Capsule)
        {
            const bool hit = TryCapsuleObbSeparation(aSecond.segmentA, aSecond.segmentB, aSecond.radius, aFirst, outSeparation, outNormal, outPenetration);
            if (hit)
            {
                outSeparation = -outSeparation;
                outNormal = -outNormal;
            }
            return hit;
        }

        return TryAabbAabbSeparation(aFirst.bounds, aSecond.bounds, outSeparation, outNormal, outPenetration);
    }

    bool Has3DOverlap(
        const CommonUtilities::AABB3D<float>& aFirstAabb,
        const CommonUtilities::AABB3D<float>& aSecondAabb)
    {
        const float overlapX =
            (std::min)(aFirstAabb.GetMax().x, aSecondAabb.GetMax().x) -
            (std::max)(aFirstAabb.GetMin().x, aSecondAabb.GetMin().x);
        const float overlapZ =
            (std::min)(aFirstAabb.GetMax().z, aSecondAabb.GetMax().z) -
            (std::max)(aFirstAabb.GetMin().z, aSecondAabb.GetMin().z);
        const float overlapY =
            (std::min)(aFirstAabb.GetMax().y, aSecondAabb.GetMax().y) -
            (std::max)(aFirstAabb.GetMin().y, aSecondAabb.GetMin().y);

        return overlapX > 0.0f && overlapY > 0.0f && overlapZ > 0.0f;
    }

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

    CollisionLayerRuleTable BuildCollisionRules()
    {
        CollisionLayerRuleTable rules;
        rules.SetSymmetric(ObjectLayer::Player, ObjectLayer::WorldStatic, CollisionRule::Block);
        rules.SetSymmetric(ObjectLayer::Enemy, ObjectLayer::WorldStatic, CollisionRule::Block);
        rules.SetSymmetric(ObjectLayer::Player, ObjectLayer::Enemy, CollisionRule::Block);
        rules.SetSymmetric(ObjectLayer::Player, ObjectLayer::Trigger, CollisionRule::Trigger);
        rules.SetSymmetric(ObjectLayer::Player, ObjectLayer::Pickup, CollisionRule::Trigger);
        rules.SetSymmetric(ObjectLayer::Player, ObjectLayer::Switch, CollisionRule::Block);
        rules.SetSymmetric(ObjectLayer::Projectile, ObjectLayer::WorldStatic, CollisionRule::Block);
        rules.SetSymmetric(ObjectLayer::Projectile, ObjectLayer::Switch, CollisionRule::Trigger);
        rules.SetSymmetric(ObjectLayer::Projectile, ObjectLayer::Enemy, CollisionRule::Block);
        return rules;
    }

    bool RequiresColliderForAudit(ObjectLayer aLayer)
    {
        return aLayer == ObjectLayer::WorldStatic ||
            aLayer == ObjectLayer::Player ||
            aLayer == ObjectLayer::Enemy;
    }

    const char* ToRuleName(CollisionRule aRule)
    {
        switch (aRule)
        {
        case CollisionRule::Ignore:
            return "Ignore";
        case CollisionRule::Block:
            return "Block";
        case CollisionRule::Trigger:
            return "Trigger";
        default:
            return "Unknown";
        }
    }

    const char* ToPhaseName(CollisionPhase aPhase)
    {
        switch (aPhase)
        {
        case CollisionPhase::Enter:
            return "Enter";
        case CollisionPhase::Stay:
            return "Stay";
        case CollisionPhase::Exit:
            return "Exit";
        default:
            return "Unknown";
        }
    }

    std::string ToString(const Vector3f& aVector)
    {
        std::ostringstream stream;
        stream << "(" << aVector.x << ", " << aVector.y << ", " << aVector.z << ")";
        return stream.str();
    }

    Vector3f GetCenter(const CommonUtilities::AABB3D<float>& anAabb)
    {
        return (anAabb.GetMin() + anAabb.GetMax()) * 0.5f;
    }

    Vector3f GetSize(const CommonUtilities::AABB3D<float>& anAabb)
    {
        return anAabb.GetMax() - anAabb.GetMin();
    }

    Vector3f GetOverlapDepths(
        const CommonUtilities::AABB3D<float>& aFirstAabb,
        const CommonUtilities::AABB3D<float>& aSecondAabb)
    {
        return Vector3f(
            (std::min)(aFirstAabb.GetMax().x, aSecondAabb.GetMax().x) -
                (std::max)(aFirstAabb.GetMin().x, aSecondAabb.GetMin().x),
            (std::min)(aFirstAabb.GetMax().y, aSecondAabb.GetMax().y) -
                (std::max)(aFirstAabb.GetMin().y, aSecondAabb.GetMin().y),
            (std::min)(aFirstAabb.GetMax().z, aSecondAabb.GetMax().z) -
                (std::max)(aFirstAabb.GetMin().z, aSecondAabb.GetMin().z));
    }

    const char* GetColliderTypeName(const GameObject& anObject)
    {
        if (anObject.GetComponent<BoxColliderComponent>())
        {
            return "Box";
        }

        if (anObject.GetComponent<SphereColliderComponent>())
        {
            return "Sphere";
        }

        if (anObject.GetComponent<CapsuleColliderComponent>())
        {
            return "Capsule";
        }

        if (anObject.GetComponent<ObbColliderComponent>())
        {
            return "OBB";
        }

        return "None";
    }

    std::string DescribeObject(const GameObject& anObject)
    {
        std::ostringstream stream;
        stream << "'" << anObject.GetName() << "'"
            << " id=" << anObject.GetCollisionId()
            << " layer=" << ToLayerName(anObject.GetLayer())
            << " collider=" << GetColliderTypeName(anObject)
            << " origin=" << ToString(anObject.GetTransform().GetPosition());
        return stream.str();
    }

    std::string DescribeAabb(const CommonUtilities::AABB3D<float>& anAabb)
    {
        std::ostringstream stream;
        stream << "min=" << ToString(anAabb.GetMin())
            << " max=" << ToString(anAabb.GetMax())
            << " center=" << ToString(GetCenter(anAabb))
            << " size=" << ToString(GetSize(anAabb));
        return stream.str();
    }
}

void RuntimeCollisionSystem::Run(std::vector<std::unique_ptr<GameObject>>& someGameObjects)
{
    const CollisionLayerRuleTable rules = BuildCollisionRules();
    myContacts.clear();

    const bool logCollisionDebug = GameDebugSettings::EnableCollisionDebugLog();
    const bool logPairChecks = GameDebugSettings::LogCollisionPairChecks();
    const bool logResolutionDetails = GameDebugSettings::LogCollisionResolutionDetails();
    int collisionDebugLogBudget = (std::max)(1, GameDebugSettings::MaxCollisionDebugLogsPerFrame());
    int skippedCollisionDebugLogs = 0;
    static std::uint64_t collisionDebugFrameIndex = 0;
    ++collisionDebugFrameIndex;

    auto logCollisionDebugLine = [&](const std::string& aText)
        {
            if (!logCollisionDebug)
            {
                return;
            }

            if (collisionDebugLogBudget <= 0)
            {
                ++skippedCollisionDebugLogs;
                return;
            }

            --collisionDebugLogBudget;
            std::cout << "[CollisionDebug][frame " << collisionDebugFrameIndex << "] " << aText << "\n";
        };

    std::unordered_map<std::uint64_t, CollisionPairState> collisionPairsThisFrame;
    std::unordered_map<std::uint64_t, GameObject*> liveColliderObjectsById;
    std::vector<GameObject*> playerObjects;
    std::vector<GameObject*> enemyObjects;
    std::vector<GameObject*> worldStaticObjects;
    std::vector<GameObject*> triggerObjects;
    std::vector<GameObject*> pickupObjects;
    std::vector<GameObject*> switchObjects;
    std::vector<GameObject*> bulletObjects;

    playerObjects.reserve(someGameObjects.size());
    enemyObjects.reserve(someGameObjects.size());
    worldStaticObjects.reserve(someGameObjects.size());
    triggerObjects.reserve(someGameObjects.size());
    pickupObjects.reserve(someGameObjects.size());
    switchObjects.reserve(someGameObjects.size());
    bulletObjects.reserve(someGameObjects.size());

    for (auto& object : someGameObjects)
    {
        if (!object || !object->IsActive() || !HasRuntimeCollider(*object))
        {
            continue;
        }

        liveColliderObjectsById.emplace(object->GetCollisionId(), object.get());

        switch (object->GetLayer())
        {
        case ObjectLayer::Player:
            playerObjects.push_back(object.get());
            break;
        case ObjectLayer::Enemy:
            enemyObjects.push_back(object.get());
            break;
        case ObjectLayer::WorldStatic:
            worldStaticObjects.push_back(object.get());
            break;
        case ObjectLayer::Trigger:
            triggerObjects.push_back(object.get());
            break;
        case ObjectLayer::Pickup:
            pickupObjects.push_back(object.get());
            break;
        case ObjectLayer::Switch:
            switchObjects.push_back(object.get());
            break;
        case ObjectLayer::Projectile:
            bulletObjects.push_back(object.get());
            break;
        default:
            break;
        }
    }

    for (auto& [id, object] : liveColliderObjectsById)
    {
        (void)id;
        RefreshRuntimeCollider(*object);
    }

    if (logCollisionDebug)
    {
        std::ostringstream stream;
        stream << "live=" << liveColliderObjectsById.size()
            << " players=" << playerObjects.size()
            << " enemies=" << enemyObjects.size()
            << " worldStatic=" << worldStaticObjects.size()
            << " triggers=" << triggerObjects.size()
            << " pickups=" << pickupObjects.size();
        logCollisionDebugLine(stream.str());
    }

    auto registerContact = [&](GameObject* aFirst, GameObject* aSecond, const Vector3f& aNormal, const float aPenetration)
        {
            const std::uint64_t pairKey = MakeCollisionPairKey(*aFirst, *aSecond);
            if (collisionPairsThisFrame.find(pairKey) != collisionPairsThisFrame.end())
            {
                return;
            }

            CollisionPairState pairState;
            pairState.firstId = aFirst->GetCollisionId();
            pairState.secondId = aSecond->GetCollisionId();
            pairState.first = aFirst;
            pairState.second = aSecond;
            collisionPairsThisFrame.emplace(pairKey, pairState);

            CollisionContact contact;
            contact.first = aFirst;
            contact.second = aSecond;
            contact.normal = aNormal;
            contact.penetration = aPenetration;
            contact.phase = myCollisionPairsLastFrame.find(pairKey) != myCollisionPairsLastFrame.end()
                ? CollisionPhase::Stay
                : CollisionPhase::Enter;
            myContacts.push_back(contact);

            if (logCollisionDebug && (logResolutionDetails || contact.phase != CollisionPhase::Stay))
            {
                std::ostringstream stream;
                stream << "contact phase=" << ToPhaseName(contact.phase)
                    << " normal=" << ToString(aNormal)
                    << " penetration=" << aPenetration
                    << " first=" << DescribeObject(*aFirst)
                    << " second=" << DescribeObject(*aSecond);
                logCollisionDebugLine(stream.str());
            }
        };

    auto resolveBlock = [&](GameObject& aDynamicObject, GameObject& anObstacleObject, const bool aRegisterContact)
        {
            const std::uint64_t pairKey = MakeCollisionPairKey(aDynamicObject, anObstacleObject);
            const bool isNewPair = myCollisionPairsLastFrame.find(pairKey) == myCollisionPairsLastFrame.end();
            const Vector3f originBefore = aDynamicObject.GetTransform().GetPosition();
            const CollisionShape dynamicShape = GetCollisionShape(aDynamicObject);
            const CollisionShape obstacleShape = GetCollisionShape(anObstacleObject);
            const CommonUtilities::AABB3D<float> dynamicAabb = dynamicShape.bounds;
            const CommonUtilities::AABB3D<float> obstacleAabb = obstacleShape.bounds;
            const Vector3f overlaps = GetOverlapDepths(dynamicAabb, obstacleAabb);
            Vector3f separation;
            Vector3f normal;
            float penetration = 0.0f;
            if (!TryComputeShapeSeparation(dynamicShape, obstacleShape, separation, normal, penetration))
            {
                if (logCollisionDebug && logPairChecks)
                {
                    std::ostringstream stream;
                    stream << "block miss rule=" << ToRuleName(CollisionRule::Block)
                        << " dynamic=" << DescribeObject(aDynamicObject)
                        << " obstacle=" << DescribeObject(anObstacleObject)
                        << " dynamicAabb={" << DescribeAabb(dynamicAabb) << "}"
                        << " obstacleAabb={" << DescribeAabb(obstacleAabb) << "}"
                        << " overlapDepths=" << ToString(overlaps);
                    logCollisionDebugLine(stream.str());
                }
                return false;
            }

            aDynamicObject.GetTransform().Translate(separation);
            RefreshRuntimeCollider(aDynamicObject);

            if (logCollisionDebug && (logResolutionDetails || isNewPair))
            {
                const CommonUtilities::AABB3D<float> resolvedAabb = GetCollisionShape(aDynamicObject).bounds;
                std::ostringstream stream;
                stream << "block hit"
                    << " dynamic=" << DescribeObject(aDynamicObject)
                    << " obstacle=" << DescribeObject(anObstacleObject)
                    << " originBefore=" << ToString(originBefore)
                    << " originAfter=" << ToString(aDynamicObject.GetTransform().GetPosition())
                    << " dynamicAabbBefore={" << DescribeAabb(dynamicAabb) << "}"
                    << " dynamicAabbAfter={" << DescribeAabb(resolvedAabb) << "}"
                    << " obstacleAabb={" << DescribeAabb(obstacleAabb) << "}"
                    << " overlapDepths=" << ToString(overlaps)
                    << " separation=" << ToString(separation)
                    << " normal=" << ToString(normal)
                    << " penetration=" << penetration;
                logCollisionDebugLine(stream.str());
            }

            if (aRegisterContact)
            {
                registerContact(&aDynamicObject, &anObstacleObject, normal, penetration);
            }

            return true;
        };

    auto registerTrigger = [&](GameObject& aFirst, GameObject& aSecond)
        {
            const CollisionShape firstShape = GetCollisionShape(aFirst);
            const CollisionShape secondShape = GetCollisionShape(aSecond);
            const CommonUtilities::AABB3D<float> firstAabb = firstShape.bounds;
            const CommonUtilities::AABB3D<float> secondAabb = secondShape.bounds;
            const Vector3f overlaps = GetOverlapDepths(firstAabb, secondAabb);
            Vector3f separation;
            Vector3f normal;
            float penetration = 0.0f;
            if (!TryComputeShapeSeparation(firstShape, secondShape, separation, normal, penetration))
            {
                if (logCollisionDebug && logPairChecks)
                {
                    std::ostringstream stream;
                    stream << "trigger miss"
                        << " first=" << DescribeObject(aFirst)
                        << " second=" << DescribeObject(aSecond)
                        << " firstAabb={" << DescribeAabb(firstAabb) << "}"
                        << " secondAabb={" << DescribeAabb(secondAabb) << "}"
                        << " overlapDepths=" << ToString(overlaps);
                    logCollisionDebugLine(stream.str());
                }
                return false;
            }

            if (logCollisionDebug)
            {
                std::ostringstream stream;
                stream << "trigger hit"
                    << " first=" << DescribeObject(aFirst)
                    << " second=" << DescribeObject(aSecond)
                    << " firstAabb={" << DescribeAabb(firstAabb) << "}"
                    << " secondAabb={" << DescribeAabb(secondAabb) << "}"
                    << " overlapDepths=" << ToString(overlaps);
                logCollisionDebugLine(stream.str());
            }

            registerContact(&aFirst, &aSecond, Vector3f::Zero, 0.0f);
            return true;
        };

    auto tryProjectileSweep = [&](GameObject& aProjectile, const std::vector<GameObject*>& someTargets)
        {
            const auto previousIt = myPreviousColliderPositionsById.find(aProjectile.GetCollisionId());
            if (previousIt == myPreviousColliderPositionsById.end())
            {
                return;
            }

            const Vector3f origin = previousIt->second;
            const Vector3f current = aProjectile.GetTransform().GetPosition();
            Vector3f delta = current - origin;
            const float distance = delta.Length();
            if (distance <= kCollisionEpsilon)
            {
                return;
            }

            const Vector3f direction = delta / distance;
            const CollisionShape projectileShape = GetCollisionShape(aProjectile);
            const float projectileRadius = GetSweepRadius(projectileShape);

            GameObject* bestTarget = nullptr;
            CollisionRaycastHit bestHit;
            bestHit.distance = distance;

            for (GameObject* target : someTargets)
            {
                if (target == nullptr || !target->IsActive())
                {
                    continue;
                }

                const CollisionRule rule = rules.Get(aProjectile.GetLayer(), target->GetLayer());
                if (rule == CollisionRule::Ignore)
                {
                    continue;
                }

                CollisionRaycastHit hit;
                if (!TryRaycastShape(
                    origin,
                    direction,
                    distance,
                    GetCollisionShape(*target),
                    projectileRadius,
                    hit))
                {
                    continue;
                }

                if (bestTarget == nullptr || hit.distance < bestHit.distance)
                {
                    bestTarget = target;
                    bestHit = hit;
                }
            }

            if (bestTarget == nullptr)
            {
                return;
            }

            aProjectile.GetTransform().SetPosition(bestHit.point);
            RefreshRuntimeCollider(aProjectile);
            registerContact(bestTarget, &aProjectile, bestHit.normal, 0.0f);

            if (bestTarget->GetLayer() == ObjectLayer::WorldStatic)
            {
                aProjectile.DisableAllComponents();
                aProjectile.SetActive(false);
            }
        };

    std::vector<GameObject*> projectileSweepTargets;
    projectileSweepTargets.reserve(worldStaticObjects.size() + switchObjects.size() + enemyObjects.size());
    projectileSweepTargets.insert(projectileSweepTargets.end(), worldStaticObjects.begin(), worldStaticObjects.end());
    projectileSweepTargets.insert(projectileSweepTargets.end(), switchObjects.begin(), switchObjects.end());
    projectileSweepTargets.insert(projectileSweepTargets.end(), enemyObjects.begin(), enemyObjects.end());

    for (GameObject* bullet : bulletObjects)
    {
        if (bullet && bullet->IsActive())
        {
            tryProjectileSweep(*bullet, projectileSweepTargets);
        }
    }

    for (GameObject* player : playerObjects)
    {
        for (int iteration = 0; iteration < kMaxCollisionIterations; ++iteration)
        {
            bool didResolve = false;
            for (GameObject* worldStatic : worldStaticObjects)
            {
                if (HasTriggerCollider(*player) || HasTriggerCollider(*worldStatic))
                {
                    registerTrigger(*player, *worldStatic);
                    continue;
                }

                if (rules.Get(player->GetLayer(), worldStatic->GetLayer()) != CollisionRule::Block)
                {
                    continue;
                }

                if (resolveBlock(*player, *worldStatic, true))
                {
                    didResolve = true;
                }
            }

            if (!didResolve)
            {
                break;
            }
        }
    }

    for (GameObject* enemy : enemyObjects)
    {
        for (int iteration = 0; iteration < kMaxCollisionIterations; ++iteration)
        {
            bool didResolve = false;
            for (GameObject* worldStatic : worldStaticObjects)
            {
                if (HasTriggerCollider(*enemy) || HasTriggerCollider(*worldStatic))
                {
                    registerTrigger(*enemy, *worldStatic);
                    continue;
                }

                if (rules.Get(enemy->GetLayer(), worldStatic->GetLayer()) != CollisionRule::Block)
                {
                    continue;
                }

                if (resolveBlock(*enemy, *worldStatic, true))
                {
                    didResolve = true;
                }
            }

            if (!didResolve)
            {
                break;
            }
        }
    }

    for (GameObject* player : playerObjects)
    {
        for (GameObject* enemy : enemyObjects)
        {
            if (HasTriggerCollider(*player) || HasTriggerCollider(*enemy))
            {
                registerTrigger(*player, *enemy);
                continue;
            }

            if (rules.Get(player->GetLayer(), enemy->GetLayer()) != CollisionRule::Block)
            {
                continue;
            }

            resolveBlock(*player, *enemy, true);
        }
    }

    for (GameObject* player : playerObjects)
    {
        for (GameObject* trigger : triggerObjects)
        {
            if (rules.Get(player->GetLayer(), trigger->GetLayer()) != CollisionRule::Trigger)
            {
                continue;
            }

            registerTrigger(*player, *trigger);
        }

        for (GameObject* pickup : pickupObjects)
        {
            if (rules.Get(player->GetLayer(), pickup->GetLayer()) != CollisionRule::Trigger)
            {
                continue;
            }

            registerTrigger(*player, *pickup);
        }
    }

    for (GameObject* enemy : enemyObjects)
    {
        for (GameObject* bullet : bulletObjects)
        {
            if (!bullet || !bullet->IsActive())
            {
                continue;
            }

            if (rules.Get(enemy->GetLayer(), bullet->GetLayer()) != CollisionRule::Block)
            {
                continue;
            }

            registerTrigger(*enemy, *bullet);
        }
    }

    for (GameObject* switchy : switchObjects)
    {
        for (GameObject* bullet : bulletObjects)
        {
            if (!bullet || !bullet->IsActive())
            {
                continue;
            }

            if (rules.Get(switchy->GetLayer(), bullet->GetLayer()) != CollisionRule::Trigger)
            {
                continue;
            }

            registerTrigger(*switchy, *bullet);
        }
    }

    for (const auto& [pairKey, pair] : myCollisionPairsLastFrame)
    {
        if (collisionPairsThisFrame.find(pairKey) != collisionPairsThisFrame.end())
        {
            continue;
        }

        auto firstIt = liveColliderObjectsById.find(pair.firstId);
        auto secondIt = liveColliderObjectsById.find(pair.secondId);
        if (firstIt == liveColliderObjectsById.end() || secondIt == liveColliderObjectsById.end())
        {
            continue;
        }

        CollisionContact contact;
        contact.first = firstIt->second;
        contact.second = secondIt->second;
        contact.phase = CollisionPhase::Exit;
        myContacts.push_back(contact);

        if (logCollisionDebug)
        {
            std::ostringstream stream;
            stream << "contact phase=Exit"
                << " first=" << DescribeObject(*contact.first)
                << " second=" << DescribeObject(*contact.second);
            logCollisionDebugLine(stream.str());
        }
    }

    if (logCollisionDebug && skippedCollisionDebugLogs > 0)
    {
        std::cout << "[CollisionDebug][frame " << collisionDebugFrameIndex << "] skipped "
            << skippedCollisionDebugLogs
            << " log lines because Collision Log Cap / Frame was reached\n";
    }

    myCollisionPairsLastFrame = std::move(collisionPairsThisFrame);

    std::unordered_map<std::uint64_t, Vector3f> previousColliderPositions;
    previousColliderPositions.reserve(liveColliderObjectsById.size());
    for (const auto& [id, object] : liveColliderObjectsById)
    {
        if (object != nullptr && object->IsActive())
        {
            previousColliderPositions.emplace(id, object->GetTransform().GetPosition());
        }
    }
    myPreviousColliderPositionsById = std::move(previousColliderPositions);
}

void RuntimeCollisionSystem::AuditRequiredColliders(const std::vector<std::unique_ptr<GameObject>>& someGameObjects) const
{
    int warningCount = 0;
    for (const auto& object : someGameObjects)
    {
        if (!object)
        {
            continue;
        }

        const ObjectLayer layer = object->GetLayer();
        if (!RequiresColliderForAudit(layer))
        {
            continue;
        }

        if (HasRuntimeCollider(*object))
        {
            continue;
        }

        ++warningCount;
        std::cout << "[CollisionAudit] WARNING: object '" << object->GetName()
            << "' on layer '" << ToLayerName(layer)
            << "' has no authored collider component.\n";
    }

    if (warningCount > 0)
    {
        std::cout << "[CollisionAudit] Total missing required colliders: " << warningCount << "\n";
    }
}

bool RuntimeCollisionSystem::Raycast(
    const std::vector<std::unique_ptr<GameObject>>& someGameObjects,
    const CollisionRaycastQuery& aQuery,
    CollisionRaycastHit& outHit) const
{
    outHit = {};

    if (aQuery.maxDistance <= 0.0f || aQuery.direction.LengthSqr() <= kCollisionEpsilon)
    {
        return false;
    }

    const Vector3f direction = aQuery.direction.GetNormalized();
    bool didHit = false;
    CollisionRaycastHit bestHit;
    bestHit.distance = aQuery.maxDistance;

    for (const auto& object : someGameObjects)
    {
        if (!object || !object->IsActive() || object.get() == aQuery.ignoredObject)
        {
            continue;
        }

        if (!aQuery.layers.Contains(object->GetLayer()) || !HasRuntimeCollider(*object))
        {
            continue;
        }

        if (!aQuery.includeTriggerColliders && HasTriggerCollider(*object))
        {
            continue;
        }

        CollisionRaycastHit hit;
        if (!TryRaycastShape(
            aQuery.origin,
            direction,
            aQuery.maxDistance,
            GetCollisionShape(*object),
            0.0f,
            hit))
        {
            continue;
        }

        if (!didHit || hit.distance < bestHit.distance)
        {
            didHit = true;
            bestHit = hit;
            bestHit.object = object.get();
            bestHit.layer = object->GetLayer();
            bestHit.collisionId = object->GetCollisionId();
            bestHit.name = object->GetName();
        }
    }

    if (!didHit)
    {
        return false;
    }

    outHit = bestHit;
    return true;
}

const std::vector<CollisionContact>& RuntimeCollisionSystem::GetContacts() const
{
    return myContacts;
}
