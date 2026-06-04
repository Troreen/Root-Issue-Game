#include "EnemyObstacleAvoidance.h"
#include "BoxColliderComponent.h"
#include "CapsuleColliderComponent.h"
#include "CollisionTypes.h"
#include "DebugSettings.h"
#include "GameObject.h"
#include "ObbColliderComponent.h"
#include "RuntimeCollisionSystem.h"
#include "SphereColliderComponent.h"
#include <algorithm>
#include <cmath>

#include <tge/drawers/LineDrawer.h>
#include <tge/engine.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/math/Matrix4x4.h>
#include <tge/math/color.h>
#include <tge/primitives/LinePrimitive.h>

namespace
{
	constexpr float AvoidanceBaseLookAhead = 240.0f;
	constexpr float AvoidanceLookAheadTime = 0.7f;
	constexpr float AvoidanceFeelAngleScale = 0.7f;
	constexpr float AvoidanceStickDuration = 0.2f;
	constexpr float AvoidanceSideDeadZone = 0.18f;
	constexpr float DefaultAvoidanceRadius = 50.0f;
	constexpr float DebugHitMarkerSize = 18.0f;

	CommonUtilities::Vector3<float> GetPlanarRight(const CommonUtilities::Vector3<float>& aForward)
	{
		return CommonUtilities::Vector3<float>(aForward.z, 0.0f, -aForward.x).GetNormalized();
	}

	void DrawDebugLine(Tga::LineDrawer& aDrawer, const Vector3f& aFrom, const Vector3f& aTo, const Tga::Color& aColor)
	{
		Tga::LinePrimitive line;
		line.fromPosition = { aFrom.x, aFrom.y, aFrom.z };
		line.toPosition = { aTo.x, aTo.y, aTo.z };
		line.color = aColor.AsVec4();
		aDrawer.Draw(line);
	}
}

CommonUtilities::Vector3<float> EnemyObstacleAvoidance::GetAvoidanceDirection(
	GameObject& anOwner,
	const CommonUtilities::Vector3<float>& aDesiredDirection,
	const float aMoveSpeed,
	const float aDeltaTime)
{
	myStickTimer = (std::max)(0.0f, myStickTimer - aDeltaTime);

	if (aDesiredDirection.LengthSqr() <= 0.001f)
	{
		myDebugRayCount = 0;
		return Vector3f::Zero;
	}

	const Vector3f desiredDirection = aDesiredDirection.GetNormalized();
	const Vector3f right = GetPlanarRight(desiredDirection);
	const float lookAhead = AvoidanceBaseLookAhead + aMoveSpeed * AvoidanceLookAheadTime;
	myDebugRayCount = 0;

	CollisionRaycastQuery query;
	query.origin = anOwner.GetTransform().GetPosition() + Vector3f(0.0f, 90.0f, 0.0f);
	query.maxDistance = lookAhead;
	query.radiusPadding = GetSweepRadius(anOwner);
	query.ignoredObject = &anOwner;
	query.includeTriggerColliders = false;
	query.layers.AddLayer(ObjectLayer::WorldStatic);
	query.layers.AddLayer(ObjectLayer::WorldDamageable);

	auto castDistance = [&](const Vector3f& aDirection, CollisionRaycastHit& outHit)
		{
			query.direction = aDirection.GetNormalized();
			if (RuntimeCollisionService::Raycast(query, outHit))
			{
				SetDebugRay(myDebugRayCount++, query, outHit, lookAhead);
				return outHit.distance;
			}

			SetDebugRay(myDebugRayCount++, query, outHit, lookAhead);
			return lookAhead;
		};

	CollisionRaycastHit forwardHit;
	CollisionRaycastHit leftHit;
	CollisionRaycastHit rightHit;
	const float forwardDistance = castDistance(desiredDirection, forwardHit);
	const float leftDistance = castDistance((desiredDirection - right * AvoidanceFeelAngleScale), leftHit);
	const float rightDistance = castDistance((desiredDirection + right * AvoidanceFeelAngleScale), rightHit);

	if (!forwardHit.HasHit() && !leftHit.HasHit() && !rightHit.HasHit())
	{
		return Vector3f::Zero;
	}

	float steer = 0.0f;
	if (forwardHit.HasHit())
	{
		const float sideDifference = rightDistance - leftDistance;
		int side = sideDifference >= 0.0f ? 1 : -1;
		if (myStickTimer > 0.0f && std::abs(sideDifference) < lookAhead * AvoidanceSideDeadZone)
		{
			side = mySide;
		}

		mySide = side;
		myStickTimer = AvoidanceStickDuration;

		const float forwardPressure = 1.0f - std::clamp(forwardDistance / lookAhead, 0.0f, 1.0f);
		steer += static_cast<float>(side) * forwardPressure;
	}

	if (leftHit.HasHit())
	{
		steer += 1.0f - std::clamp(leftDistance / lookAhead, 0.0f, 1.0f);
	}
	if (rightHit.HasHit())
	{
		steer -= 1.0f - std::clamp(rightDistance / lookAhead, 0.0f, 1.0f);
	}

	steer = std::clamp(steer, -1.0f, 1.0f);
	if (std::abs(steer) <= 0.001f)
	{
		return Vector3f::Zero;
	}

	return right * steer;
}

void EnemyObstacleAvoidance::RenderDebug() const
{
#ifndef _RETAIL
	if (!GameDebugSettings::ShowEnemyAvoidanceDebugLines() || myDebugRayCount <= 0)
	{
		return;
	}

	Tga::Engine* engine = Tga::Engine::GetInstance();
	if (!engine)
	{
		return;
	}

	auto& graphicsEngine = engine->GetGraphicsEngine();
	Tga::GraphicsStateStack& graphicsStateStack = graphicsEngine.GetGraphicsStateStack();
	Tga::LineDrawer& drawer = graphicsEngine.GetLineDrawer();

	graphicsStateStack.Push();
	graphicsStateStack.SetTransform(Tga::Matrix4x4f::CreateIdentityMatrix());

	for (int index = 0; index < myDebugRayCount; ++index)
	{
		const DebugRay& ray = myDebugRays[index];
		if (ray.Direction.LengthSqr() <= 0.001f || ray.Distance <= 0.0f)
		{
			continue;
		}

		const Vector3f direction = ray.Direction.GetNormalized();
		const Vector3f right = GetPlanarRight(direction);
		const Vector3f end = ray.Origin + direction * ray.Distance;
		const Tga::Color color = ray.Hit
			? Tga::Color{ 1.0f, 0.2f, 0.05f, 1.0f }
			: Tga::Color{ 0.2f, 1.0f, 0.25f, 1.0f };
		const Tga::Color railColor = ray.Hit
			? Tga::Color{ 1.0f, 0.55f, 0.05f, 1.0f }
			: Tga::Color{ 0.1f, 0.7f, 1.0f, 1.0f };

		DrawDebugLine(drawer, ray.Origin, end, color);
		DrawDebugLine(drawer, ray.Origin + right * ray.Radius, end + right * ray.Radius, railColor);
		DrawDebugLine(drawer, ray.Origin - right * ray.Radius, end - right * ray.Radius, railColor);

		if (ray.Hit)
		{
			DrawDebugLine(drawer, end - right * DebugHitMarkerSize, end + right * DebugHitMarkerSize, color);
			DrawDebugLine(drawer, end - Vector3f::UnitY * DebugHitMarkerSize, end + Vector3f::UnitY * DebugHitMarkerSize, color);
		}
	}

	graphicsStateStack.Pop();
#endif
}

float EnemyObstacleAvoidance::GetSweepRadius(GameObject& anOwner) const
{
	if (const CapsuleColliderComponent* capsule = anOwner.GetComponent<CapsuleColliderComponent>())
	{
		return capsule->GetRadius();
	}
	if (const SphereColliderComponent* sphere = anOwner.GetComponent<SphereColliderComponent>())
	{
		return sphere->GetRadius();
	}
	if (const BoxColliderComponent* box = anOwner.GetComponent<BoxColliderComponent>())
	{
		const Vector3f& size = box->GetSize();
		return (std::max)(size.x, size.z) * 0.5f;
	}
	if (const ObbColliderComponent* obb = anOwner.GetComponent<ObbColliderComponent>())
	{
		const Vector3f halfExtents = obb->GetHalfExtents();
		return (std::max)(halfExtents.x, halfExtents.z);
	}

	return DefaultAvoidanceRadius;
}

void EnemyObstacleAvoidance::SetDebugRay(
	const int anIndex,
	const CollisionRaycastQuery& aQuery,
	const CollisionRaycastHit& aHit,
	const float aFallbackDistance)
{
	if (anIndex < 0 || anIndex >= 3)
	{
		return;
	}

	DebugRay& ray = myDebugRays[anIndex];
	ray.Origin = aQuery.origin;
	ray.Direction = aQuery.direction;
	ray.Radius = aQuery.radiusPadding;
	ray.Hit = aHit.HasHit();
	ray.Distance = ray.Hit ? aHit.distance : aFallbackDistance;
}
