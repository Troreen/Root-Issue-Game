#include "WorldTriggerHelpers.h"

#include "BoxColliderComponent.h"
#include "CapsuleColliderComponent.h"
#include "GameObject.h"
#include "ObbColliderComponent.h"
#include "SphereColliderComponent.h"

namespace WorldTriggerHelpers
{
	bool IsTriggerInside(const GameObject& anObject)
	{
		if (const auto* box = anObject.GetComponent<BoxColliderComponent>())
		{
			if (box->IsTrigger() && box->IsInside())
			{
				return true;
			}
		}

		if (const auto* sphere = anObject.GetComponent<SphereColliderComponent>())
		{
			if (sphere->IsTrigger() && sphere->IsInside())
			{
				return true;
			}
		}

		if (const auto* capsule = anObject.GetComponent<CapsuleColliderComponent>())
		{
			if (capsule->IsTrigger() && capsule->IsInside())
			{
				return true;
			}
		}

		if (const auto* obb = anObject.GetComponent<ObbColliderComponent>())
		{
			if (obb->IsTrigger() && obb->IsInside())
			{
				return true;
			}
		}

		return false;
	}

	bool TryGetColliderAabb(const GameObject& anObject, CommonUtilities::AABB3D<float>& outAabb)
	{
		if (const auto* box = anObject.GetComponent<BoxColliderComponent>())
		{
			outAabb = box->GetAABB();
			return true;
		}

		if (const auto* sphere = anObject.GetComponent<SphereColliderComponent>())
		{
			outAabb = sphere->GetAABB();
			return true;
		}

		if (const auto* capsule = anObject.GetComponent<CapsuleColliderComponent>())
		{
			outAabb = capsule->GetAABB();
			return true;
		}

		if (const auto* obb = anObject.GetComponent<ObbColliderComponent>())
		{
			outAabb = obb->GetAABB();
			return true;
		}

		return false;
	}

	Vector3f GetTriggerCenter(const GameObject& anObject)
	{
		CommonUtilities::AABB3D<float> aabb(Vector3f::Zero, Vector3f::Zero);
		if (!TryGetColliderAabb(anObject, aabb))
		{
			return anObject.GetTransform().GetPosition();
		}

		return (aabb.GetMin() + aabb.GetMax()) * 0.5f;
	}

	Vector3f GetTriggerHalfExtents(const GameObject& anObject)
	{
		CommonUtilities::AABB3D<float> aabb(Vector3f::Zero, Vector3f::Zero);
		if (!TryGetColliderAabb(anObject, aabb))
		{
			return Vector3f::Zero;
		}

		return (aabb.GetMax() - aabb.GetMin()) * 0.5f;
	}

	void ForceColliderToTrigger(GameObject& anObject)
	{
		if (auto* box = anObject.GetComponent<BoxColliderComponent>())
		{
			box->SetIsTrigger(true);
		}

		if (auto* sphere = anObject.GetComponent<SphereColliderComponent>())
		{
			sphere->SetIsTrigger(true);
		}

		if (auto* capsule = anObject.GetComponent<CapsuleColliderComponent>())
		{
			capsule->SetIsTrigger(true);
		}

		if (auto* obb = anObject.GetComponent<ObbColliderComponent>())
		{
			obb->SetIsTrigger(true);
		}
	}

	void AddDefaultBoxTrigger(GameObject& anObject, const Vector3f& aSize)
	{
		anObject.AddComponent<BoxColliderComponent>(aSize, Vector3f::Zero, true, true, true);
	}
}
