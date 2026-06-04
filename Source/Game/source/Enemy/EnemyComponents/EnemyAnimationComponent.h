#pragma once
#include "ScriptComponent.h"
#include <unordered_map>
#include <string>
#include "EnemyData.h"

class AnimationGraphComponent;

enum class EnemyAnimationState
{
	Spawn,
	Idle,
	IdleAggro,
	Aggro,
	Walk,
	AggroWalk,
	Attack,
	ChargeAttack,
	RollIdle,
	Hurt,
	Block,
	KnockDown,
	Death,
	DeathLaying,
	DeathStanding

};

class EnemyAnimationComponent : public ScriptComponent
{
public:

	EnemyAnimationComponent() = default;
	//EnemyAnimationComponent(const EnemyData& someData);

	void OnStart() override;
	void OnUpdate(float aDeltaTime) override;

	void BlendTo(EnemyAnimationState aState, float aBlendSpeed = 2.0f);

private:

	struct AnimationEntry
	{
		std::string Name;
		float Weight;
	};

	void RegisterAnimations();
	void UpdateBlendWeights(float aDeltaTime);

	EnemyAnimationState myCurrentState;

	AnimationGraphComponent* myGraph = nullptr;

	std::unordered_map<EnemyAnimationState, AnimationEntry> myAnimations;

	float myBlendSpeed;
};

