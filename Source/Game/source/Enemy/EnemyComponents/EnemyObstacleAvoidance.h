#pragma once
#include <Vector.hpp>

class GameObject;

class EnemyObstacleAvoidance
{
public:
	CommonUtilities::Vector3<float> GetAvoidanceDirection(
		GameObject& anOwner,
		const CommonUtilities::Vector3<float>& aDesiredDirection,
		float aMoveSpeed,
		float aDeltaTime);

	void RenderDebug() const;

private:
	struct DebugRay
	{
		CommonUtilities::Vector3<float> Origin = { 0.0f, 0.0f, 0.0f };
		CommonUtilities::Vector3<float> Direction = { 0.0f, 0.0f, 1.0f };
		float Distance = 0.0f;
		float Radius = 0.0f;
		bool Hit = false;
	};

	float GetSweepRadius(GameObject& anOwner) const;
	void SetDebugRay(int anIndex, const struct CollisionRaycastQuery& aQuery, const struct CollisionRaycastHit& aHit, float aFallbackDistance);

	float myStickTimer = 0.0f;
	int mySide = 1;
	DebugRay myDebugRays[3];
	int myDebugRayCount = 0;
};
