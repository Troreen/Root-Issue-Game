#include "CollisionQuery.h"

#include "BoxColliderComponent.h"
#include "CapsuleColliderComponent.h"
#include "GameObject.h"
#include "ObbColliderComponent.h"
#include "SphereColliderComponent.h"

#include <algorithm>
#include <cmath>

namespace CollisionQuery
{
    namespace
    {
        constexpr float kEpsilon = 0.0001f;

        float Clamp(float aValue, float aMin, float aMax)
        {
            return (std::max)(aMin, (std::min)(aValue, aMax));
        }

        bool Has3DOverlap(
            const CommonUtilities::AABB3D<float>& aFirst,
            const CommonUtilities::AABB3D<float>& aSecond)
        {
            const float overlapX = (std::min)(aFirst.GetMax().x, aSecond.GetMax().x) -
                (std::max)(aFirst.GetMin().x, aSecond.GetMin().x);
            const float overlapY = (std::min)(aFirst.GetMax().y, aSecond.GetMax().y) -
                (std::max)(aFirst.GetMin().y, aSecond.GetMin().y);
            const float overlapZ = (std::min)(aFirst.GetMax().z, aSecond.GetMax().z) -
                (std::max)(aFirst.GetMin().z, aSecond.GetMin().z);
            return overlapX > 0.0f && overlapY > 0.0f && overlapZ > 0.0f;
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
            if (lengthSquared <= kEpsilon)
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
            const float overlapX = (std::min)(aFirstAabb.GetMax().x, aSecondAabb.GetMax().x) -
                (std::max)(aFirstAabb.GetMin().x, aSecondAabb.GetMin().x);
            const float overlapY = (std::min)(aFirstAabb.GetMax().y, aSecondAabb.GetMax().y) -
                (std::max)(aFirstAabb.GetMin().y, aSecondAabb.GetMin().y);
            const float overlapZ = (std::min)(aFirstAabb.GetMax().z, aSecondAabb.GetMax().z) -
                (std::max)(aFirstAabb.GetMin().z, aSecondAabb.GetMin().z);

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
                outNormal.x = firstCenter.x < secondCenter.x ? -1.0f : 1.0f;
            }
            else if (overlapY <= overlapZ)
            {
                outPenetration = overlapY;
                outNormal.y = firstCenter.y < secondCenter.y ? -1.0f : 1.0f;
            }
            else
            {
                outPenetration = overlapZ;
                outNormal.z = firstCenter.z < secondCenter.z ? -1.0f : 1.0f;
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
            if (distance > kEpsilon)
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

            return TryAabbAabbSeparation(
                CommonUtilities::AABB3D<float>(
                    aSphereCenter - Vector3f(aSphereRadius, aSphereRadius, aSphereRadius),
                    aSphereCenter + Vector3f(aSphereRadius, aSphereRadius, aSphereRadius)),
                anAabb,
                outSeparation,
                outNormal,
                outPenetration);
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
            const Vector3f aabbCenter = (anAabb.GetMin() + anAabb.GetMax()) * 0.5f;
            const Vector3f capsulePoint = ClosestPointOnSegment(aabbCenter, aCapsuleStart, aCapsuleEnd);
            const Vector3f boxPoint = ClosestPointOnAabb(capsulePoint, anAabb);
            Vector3f delta = capsulePoint - boxPoint;
            const float distance = delta.Length();
            if (distance > kEpsilon)
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

            const float minY = (std::min)(aCapsuleStart.y, aCapsuleEnd.y);
            const float maxY = (std::max)(aCapsuleStart.y, aCapsuleEnd.y);
            return TryAabbAabbSeparation(
                CommonUtilities::AABB3D<float>(
                    Vector3f(aCapsuleStart.x - aCapsuleRadius, minY - aCapsuleRadius, aCapsuleStart.z - aCapsuleRadius),
                    Vector3f(aCapsuleStart.x + aCapsuleRadius, maxY + aCapsuleRadius, aCapsuleStart.z + aCapsuleRadius)),
                anAabb,
                outSeparation,
                outNormal,
                outPenetration);
        }
    }

    bool HasRuntimeCollider(const GameObject& anObject)
    {
        return anObject.GetComponent<BoxColliderComponent>() != nullptr ||
            anObject.GetComponent<SphereColliderComponent>() != nullptr ||
            anObject.GetComponent<CapsuleColliderComponent>() != nullptr ||
            anObject.GetComponent<ObbColliderComponent>() != nullptr;
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

    Shape MakeBoxShape(const Vector3f& aCenter, const Vector3f& aSize)
    {
        Shape shape;
        shape.type = CollisionShapeType::Box;
        shape.center = aCenter;
        shape.halfExtents = aSize * 0.5f;
        shape.bounds = CommonUtilities::AABB3D<float>(aCenter - shape.halfExtents, aCenter + shape.halfExtents);
        shape.isValid = aSize.x > 0.0f && aSize.y > 0.0f && aSize.z > 0.0f;
        return shape;
    }

    Shape GetShape(const GameObject& anObject)
    {
        Shape shape;

        if (const auto* box = anObject.GetComponent<BoxColliderComponent>())
        {
            shape = MakeBoxShape((box->GetAABB().GetMin() + box->GetAABB().GetMax()) * 0.5f, box->GetAABB().GetMax() - box->GetAABB().GetMin());
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

    bool TryComputeSeparation(
        const Shape& aFirst,
        const Shape& aSecond,
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

        return TryAabbAabbSeparation(aFirst.bounds, aSecond.bounds, outSeparation, outNormal, outPenetration);
    }
}
