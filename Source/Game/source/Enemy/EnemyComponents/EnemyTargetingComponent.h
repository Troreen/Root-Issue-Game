#pragma once
#include "ScriptComponent.h"

class EnemyAIComponent;

class EnemyTargetingComponent : public ScriptComponent
{
public:

	EnemyTargetingComponent();
	~EnemyTargetingComponent();

	void OnUpdate(float aDeltaTime) override;

	bool IsTargetInRange() const;

private:

	float myDetectionRange = 500.0f;
	bool myTargetIsInRange;

};

