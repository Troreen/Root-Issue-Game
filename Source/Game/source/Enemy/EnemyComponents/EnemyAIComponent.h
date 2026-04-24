#pragma once
#include "ScriptComponent.h"
#include "EnemyData.h"

enum class BasicEnemyState
{
	Idle,
	Walking,
	Chasing,
	Death
};

class EnemyAIComponent : public ScriptComponent
{
public:

	EnemyAIComponent() = default;
	EnemyAIComponent(EnemyType aEnemyType);
	~EnemyAIComponent();

	void OnStart() override;
	void OnUpdate(float aDeltaTime) override;

	void SetAggro(bool aState);

private:

	void BasicEnemyLogicUpdate(float aDeltaTime);
	void RollingEnemyLogicUpdate(float aDeltaTime);

	bool myIsAggro = false;
	//void OnEnter();
	//void OnExit();

	void AILogicUpdate(float aDeltaTime);

	EnemyType myType;

	BasicEnemyState myCurrentState;
	BasicEnemyState myPreviousState;

};

