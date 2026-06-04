#pragma once
#include "ScriptComponent.h"
#include "EnemyAIComponent.h"
#include <vector>

class SpanwnerComponent : public ScriptComponent
{
public:

	SpanwnerComponent();

	void OnStart() override;
	void OnUpdate(float) override;
	void Reset() override;

	void SetRadius(float aRadius);

private:

	std::vector<EnemyAIComponent*> myEnemies;
	int myIndex;
	float myTriggerRadius;
	bool myIsTrigger;
};