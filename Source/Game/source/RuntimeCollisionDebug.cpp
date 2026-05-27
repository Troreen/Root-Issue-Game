#include "RuntimeCollisionDebug.h"

#include "GameObject.h"
#include "RuntimeCollisionLayers.h"
#include "RuntimeCollisionShapes.h"

#include <sstream>

namespace RuntimeCollision
{
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
