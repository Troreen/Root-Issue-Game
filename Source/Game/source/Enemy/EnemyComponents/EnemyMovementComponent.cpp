#include "EnemyMovementComponent.h"
#include "GameObject.h"
#include <random>

void EnemyMovementComponent::OnStart()
{
}

void EnemyMovementComponent::OnUpdate(float /*aDeltaTime*/)
{
}

void EnemyMovementComponent::MoveTowards(GameObject* aTarget, float aDeltaTime)
{
	auto& transform = GetOwner()->GetTransform();
	const Vector3f& targetPos = aTarget->GetTransform().GetPosition();
	Vector3f diff = targetPos - transform.GetPosition();
	Vector3f direction = diff.GetNormalized();

	RotateTowardsTarget(aTarget);

	transform.Translate(direction * mySpeed * aDeltaTime);
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
		float randomAngle = GetRandomAngleDegreeToRad(-180.0f, 180.0f);

		CommonUtilities::Quaternion<float> turnRotation = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(Vector3f::UnitY, randomAngle);

		CommonUtilities::Quaternion<float> oldRotation = transform.GetRotation();

		CommonUtilities::Quaternion<float> newRotation = oldRotation * turnRotation;

		transform.SetRotation(newRotation);

		myMoveDistance = 0.0f;
	}
}

void EnemyMovementComponent::RotateTowardsTarget(GameObject* aTarget)
{
	auto& transform = GetOwner()->GetTransform();
	const Vector3f& targetPos = aTarget->GetTransform().GetPosition();

	Vector3f diff = targetPos - transform.GetPosition();

	float angle = std::atan2(diff.x, diff.z);

	CommonUtilities::Quaternion<float> rotation = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(Vector3f::UnitY, angle);

	transform.SetRotation(rotation);
}

float EnemyMovementComponent::GetRandomAngleDegreeToRad(float aMin, float aMax)
{
	float pi = 3.14159f;
	float minRadians = aMin * pi / 180.0f;
	float maxRadians = aMax * pi / 180.0f;

	std::random_device seed;
	std::mt19937 rndEngine(seed());
	std::uniform_real_distribution<float> rndDist(minRadians, maxRadians);

	return rndDist(rndEngine);
}
