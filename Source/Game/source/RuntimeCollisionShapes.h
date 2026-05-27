#pragma once

#include "RuntimeCollisionTypes.h"

#include <cstdint>

class GameObject;

namespace RuntimeCollision
{
    bool HasRuntimeCollider(const GameObject& anObject);
    bool HasTriggerCollider(const GameObject& anObject);
    void RefreshRuntimeCollider(GameObject& anObject);
    CollisionShape GetCollisionShape(const GameObject& anObject);
    const char* GetColliderTypeName(const GameObject& anObject);
}
