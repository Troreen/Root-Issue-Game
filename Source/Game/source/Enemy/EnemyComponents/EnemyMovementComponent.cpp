#include "EnemyMovementComponent.h"
#include "GameObject.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
	constexpr float Pi = 3.14159265358979323846f;
	constexpr float AvoidanceStrength = 1.25f;
	constexpr float BaseChaseTurnSpeed = 5.0f;
	constexpr float MaxChaseTurnSpeed = 14.0f;
}

void EnemyMovementComponent::OnStart()
{
	std::random_device seed;
	std::mt19937 rndEngine(seed());

	myRandomEngine = rndEngine;
}

void EnemyMovementComponent::OnUpdate(float /*aDeltaTime*/)
{
	//auto& pos = GetOwner()->GetTransform().GetPosition();

	//std::cout << "Position: " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
}

void EnemyMovementComponent::Render()
{
	myObstacleAvoidance.RenderDebug();
}

void EnemyMovementComponent::MoveTowardsTarget(GameObject* aTarget, float aDeltaTime)
{
	auto& transform = GetOwner()->GetTransform();
	const Vector3f& targetPos = aTarget->GetTransform().GetPosition();
	Vector3f diff = targetPos - transform.GetPosition();
	diff.y = 0.0f;

	if (diff.Length() < 0.001f)
	{
		myVelocity = Vector3f(0.0f, 0.0f, 0.0f);
		return;
	}

	Vector3f direction = diff.GetNormalized();
	const Vector3f avoidance = myObstacleAvoidance.GetAvoidanceDirection(*GetOwner(), direction, mySpeed, aDeltaTime);
	const float avoidanceAmount = std::clamp(avoidance.Length(), 0.0f, 1.0f);
	if (avoidance.LengthSqr() > 0.001f)
	{
		direction = (direction + avoidance * AvoidanceStrength).GetNormalized();
	}

	RotateTowards(direction, aDeltaTime, GetAdaptiveTurnSpeed(direction, avoidanceAmount));

	myVelocity = direction * mySpeed;

	transform.Translate(myVelocity * aDeltaTime);
}

void EnemyMovementComponent::MoveRandomly(float aDeltaTime)
{
	auto& transform = GetOwner()->GetTransform();

	if (myMoveDistance < myTargetDistance)
	{
		Vector3f forward = transform.GetForward();
		transform.Translate(forward * mySpeed * aDeltaTime);
		myMoveDistance += mySpeed * aDeltaTime;
	}
	else
	{
		/*float rotationSpeed = 2.0f;
		float t = rotationSpeed * aDeltaTime;*/

		float randomAngle = GetRandomAngleDegreeToRad(-180.0f, 180.0f);

		CommonUtilities::Quaternion<float> turnRotation = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(Vector3f::UnitY, randomAngle);

		CommonUtilities::Quaternion<float> oldRotation = transform.GetRotation();

		CommonUtilities::Quaternion<float> newRotation = oldRotation * turnRotation;

		/*CommonUtilities::Quaternion<float> targetRotation = oldRotation * turnRotation;

		CommonUtilities::Quaternion<float> newRotation = Quaternion::Slerp(oldRotation, targetRotation, t);*/

		transform.SetRotation(newRotation);

		myMoveDistance = 0.0f;
	}
}

void EnemyMovementComponent::MoveForward(float aDeltaTime)
{
	auto& transform = GetOwner()->GetTransform();
	Vector3f forward = transform.GetForward();

	myVelocity = forward * mySpeed;
	transform.Translate(myVelocity * aDeltaTime);
}

const CommonUtilities::Vector3<float>& EnemyMovementComponent::GetVelocity() const
{
	return myVelocity;
}

void EnemyMovementComponent::StopMoving()
{
	myVelocity = Vector3f(0.0f, 0.0f, 0.0f);
}

void EnemyMovementComponent::SetMovementSpeed(float aMoveSpeed)
{
	mySpeed = aMoveSpeed;
}

void EnemyMovementComponent::RotateTowards(const CommonUtilities::Vector3<float>& aDirection, float aDeltaTime)
{
	RotateTowards(aDirection, aDeltaTime, BaseChaseTurnSpeed);
}

float EnemyMovementComponent::GetAdaptiveTurnSpeed(const CommonUtilities::Vector3<float>& aDirection, const float anAvoidanceAmount) const
{
	GameObject* owner = GetOwner();
	if (!owner || aDirection.LengthSqr() <= 0.001f)
	{
		return BaseChaseTurnSpeed;
	}

	Vector3f forward = owner->GetTransform().GetForward();
	forward.y = 0.0f;
	if (forward.LengthSqr() <= 0.001f)
	{
		return BaseChaseTurnSpeed;
	}

	const Vector3f direction = aDirection.GetNormalized();
	forward.Normalize();
	const float dot = std::clamp(forward.Dot(direction), -1.0f, 1.0f);
	const float angleFactor = std::acos(dot) / Pi;
	const float avoidanceBoost = std::clamp(anAvoidanceAmount, 0.0f, 1.0f) * 0.35f;
	const float speedFactor = std::clamp(angleFactor + avoidanceBoost, 0.0f, 1.0f);
	return BaseChaseTurnSpeed + (MaxChaseTurnSpeed - BaseChaseTurnSpeed) * speedFactor;
}

void EnemyMovementComponent::RotateTowards(const CommonUtilities::Vector3<float>& aDirection, float aDeltaTime, const float aTurnSpeed)
{
	auto& transform = GetOwner()->GetTransform();

	float t = std::clamp(aTurnSpeed * aDeltaTime, 0.0f, 1.0f);

	float angle = std::atan2(aDirection.x, aDirection.z);

	CommonUtilities::Quaternion<float> turnRotation = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(Vector3f::UnitY, angle);

	CommonUtilities::Quaternion<float> oldRotation = transform.GetRotation();

	CommonUtilities::Quaternion<float> newRotation = CommonUtilities::Quaternion<float>::Slerp(oldRotation, turnRotation, t);

	transform.SetRotation(newRotation);
}

float EnemyMovementComponent::GetRandomAngleDegreeToRad(float aMin, float aMax)
{
	float pi = 3.14159f;
	float minRadians = aMin * pi / 180.0f;
	float maxRadians = aMax * pi / 180.0f;

	std::uniform_real_distribution<float> rndDist(minRadians, maxRadians);

	return rndDist(myRandomEngine);
}
