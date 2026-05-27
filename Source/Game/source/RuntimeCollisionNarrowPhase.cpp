#include "RuntimeCollisionNarrowPhase.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace RuntimeCollision
{
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
}
