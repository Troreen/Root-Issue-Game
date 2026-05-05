#include "EnemyMovementComponent.h"
#include "GameObject.h"
#include <iostream>

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

void EnemyMovementComponent::MoveTowardsTarget(GameObject* aTarget, float aDeltaTime)
{
	auto& transform = GetOwner()->GetTransform();
	const Vector3f& targetPos = aTarget->GetTransform().GetPosition();
	Vector3f diff = targetPos - transform.GetPosition();
	diff.y = 0.0f;

	if (diff.Length() < 0.001f)
	{
		myVelocity = { 0.0f, 0.0f, 0.0f };
		return;
	}

	Vector3f direction = diff.GetNormalized();

	RotateTowards(direction, aDeltaTime);

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
	myVelocity = { 0.0f, 0.0f, 0.0f };
}

void EnemyMovementComponent::RotateTowards(const CommonUtilities::Vector3<float>& aDirection, float aDeltaTime)
{
	auto& transform = GetOwner()->GetTransform();

	float rotationSpeed = 5.0f;
	float t = std::clamp(rotationSpeed * aDeltaTime, 0.0f, 1.0f);

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
