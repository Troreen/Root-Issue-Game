#pragma once
#include "ScriptComponent.h"
#include <unordered_map>
#include <string>

class AnimationGraphComponent;

enum class PlayerAnimationState
{
	None,
	Walk,
	Attack1,
	Attack2,
	Attack3,
	AttackInterrupt,
	Hurt,
	RangeAttack,
	RangeCharge,
	RangeChargeIdle,
	Upgrade,
	FusePickup,
	AntennaPlace,
	Death
};

class PlayerAnimationComponent : public ScriptComponent
{
public:

	PlayerAnimationComponent() = default;

	void OnStart() override;
	void OnUpdate(float aDeltaTime) override;

	void BlendTo(PlayerAnimationState aState, float aBlendSpeed = 2.0f);

private:

	struct AnimationEntry
	{
		std::string Name;
		float Weight;
	};

	void RegisterAnimations();
	void UpdateBlendWeights(float aDeltaTime);

	PlayerAnimationState myCurrentState = PlayerAnimationState::None;

	AnimationGraphComponent* myGraph = nullptr;

	std::unordered_map<PlayerAnimationState, AnimationEntry> myAnimations;

	float myBlendSpeed = 2.0f;
};

