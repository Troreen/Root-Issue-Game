#pragma once
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

	void MoveTowardsTarget(GameObject* aTarget, float aDeltaTime);
	void RotateTowards(const CommonUtilities::Vector3<float>& aDirection, float aDeltaTime);
	void MoveForward(float aDeltaTime);
	void StopMoving();

	// Temporary
	void MoveRandomly(float aDeltaTime);


	const CommonUtilities::Vector3<float>& GetVelocity() const;


private:

	float GetRandomAngleDegreeToRad(float aMin, float aMax);

	float mySpeed = 200.0f;
	float myMoveDistance = 0.0f;
	float myTargetDistance = 500.0f;
	CommonUtilities::Vector3<float> myVelocity = { 0.0f, 0.0f, 0.0f };

	std::mt19937 myRandomEngine;
};

