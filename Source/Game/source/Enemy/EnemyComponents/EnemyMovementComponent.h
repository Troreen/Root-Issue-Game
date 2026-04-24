#pragma once
#include "ScriptComponent.h"

class EnemyMovementComponent : public ScriptComponent
{
public:

	EnemyMovementComponent() = default;
	~EnemyMovementComponent() {}

	void OnStart() override;
	void OnUpdate(float aDeltaTime) override;
	void MoveTowards(GameObject* aTarget, float aDeltaTime);
	void MoveRandomly(float aDeltaTime);

private:

	void RotateTowardsTarget(GameObject* aTarget);
	float GetRandomAngleDegreeToRad(float aMin, float aMax);

	float myMoveTimer = 0.0f;
	float myMoveRate = 5.0f;
	float mySpeed = 200.0f;
	float myMoveDistance = 0.0f;
	float myTargetDistance = 500.0f;
};

