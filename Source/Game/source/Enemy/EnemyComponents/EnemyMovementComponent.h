#pragma once
#include "EnemyObstacleAvoidance.h"
#include "ScriptComponent.h"
#include <random>
#include <Vector.hpp>

class EnemyMovementComponent : public ScriptComponent
{
public:

	EnemyMovementComponent() = default;
	~EnemyMovementComponent() {}

	void OnStart() override;
	void OnUpdate(float aDeltaTime) override;
	void Render() override;

	void MoveTowardsTarget(GameObject* aTarget, float aDeltaTime);
	void RotateTowards(const CommonUtilities::Vector3<float>& aDirection, float aDeltaTime);
	void MoveForward(float aDeltaTime);
	void StopMoving();

	void SetMovementSpeed(float aMoveSpeed);

	// Temporary
	void MoveRandomly(float aDeltaTime);


	const CommonUtilities::Vector3<float>& GetVelocity() const;


private:

	float GetAdaptiveTurnSpeed(const CommonUtilities::Vector3<float>& aDirection, float anAvoidanceAmount) const;
	void RotateTowards(const CommonUtilities::Vector3<float>& aDirection, float aDeltaTime, float aTurnSpeed);
	float GetRandomAngleDegreeToRad(float aMin, float aMax);

	float mySpeed;
	float myMoveDistance = 0.0f;
	float myTargetDistance = 500.0f;
	EnemyObstacleAvoidance myObstacleAvoidance;
	CommonUtilities::Vector3<float> myVelocity = { 0.0f, 0.0f, 0.0f };

	std::mt19937 myRandomEngine;
};

