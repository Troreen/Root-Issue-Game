#pragma once

#include <CommonUtilities/AABB3D.hpp>
#include <CommonUtilities/Vector3.hpp>

class GameObject;

namespace WorldTriggerHelpers
{
	using Vector3f = CommonUtilities::Vector3<float>;

	bool IsTriggerInside(const GameObject& anObject);
	bool TryGetColliderAabb(const GameObject& anObject, CommonUtilities::AABB3D<float>& outAabb);
	Vector3f GetTriggerCenter(const GameObject& anObject);
	Vector3f GetTriggerHalfExtents(const GameObject& anObject);
	void ForceColliderToTrigger(GameObject& anObject);
	void AddDefaultBoxTrigger(GameObject& anObject, const Vector3f& aSize);
}
