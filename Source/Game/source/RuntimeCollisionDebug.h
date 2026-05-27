#pragma once

#include "RuntimeCollisionTypes.h"

#include <string>

class GameObject;

namespace RuntimeCollision
{
    std::string ToString(const Vector3f& aVector);
    std::string DescribeObject(const GameObject& anObject);
    std::string DescribeAabb(const CommonUtilities::AABB3D<float>& anAabb);
}
