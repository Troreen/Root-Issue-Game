#pragma once
#include "ScriptComponent.h"

class EnemyAIComponent;

class EnemyTargetingComponent : public ScriptComponent
{
public:

	EnemyTargetingComponent() = delete;
	EnemyTargetingComponent(float aDetectionRange);
	~EnemyTargetingComponent();

	void OnUpdate(float aDeltaTime) override;

	bool IsTargetInRange() const;

private:

	float myDetectionRange;
	bool myTargetIsInRange;

};

