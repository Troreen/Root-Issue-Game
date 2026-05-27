#include "RuntimeCollisionShapes.h"

#include "BoxColliderComponent.h"
#include "CapsuleColliderComponent.h"
#include "GameObject.h"
#include "ObbColliderComponent.h"
#include "SphereColliderComponent.h"

namespace RuntimeCollision
{
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
}
