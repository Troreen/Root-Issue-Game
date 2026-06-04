#include "PlayerAnimationComponent.h"
#include "AnimationGraphComponent.h"
#include "GameObject.h"

void PlayerAnimationComponent::OnStart()
{
	myGraph = GetOwner()->GetComponent<AnimationGraphComponent>();

	if (!myGraph)
	{
		std::cout << "AnimGraph missing!\n";
		return;
	}

	myBlendSpeed = 2.0f;

	RegisterAnimations();
}

void PlayerAnimationComponent::OnUpdate(float aDeltaTime)
{
	if (!myGraph)
	{
		return;
	}

	UpdateBlendWeights(aDeltaTime);
}

void PlayerAnimationComponent::BlendTo(PlayerAnimationState aState, float aBlendSpeed)
{
	myCurrentState = aState;
	myBlendSpeed = aBlendSpeed;
}

void PlayerAnimationComponent::RegisterAnimations()
{
	myAnimations[PlayerAnimationState::Walk]			= { "w_walk", 0.0f };
	myAnimations[PlayerAnimationState::Attack1]			= { "w_attack_basic01", 0.0f };
	myAnimations[PlayerAnimationState::Attack2]			= { "w_attack_basic02" , 0.0f };
	myAnimations[PlayerAnimationState::Attack3]			= { "w_attack_basic03", 0.0f };
	myAnimations[PlayerAnimationState::AttackInterrupt]	= { "w_attack_interrupt", 0.0f };
	myAnimations[PlayerAnimationState::Hurt]			= { "w_hurt", 0.0f };
	myAnimations[PlayerAnimationState::RangeAttack]		= { "w_ranged_attack", 0.0f };
	myAnimations[PlayerAnimationState::RangeCharge]		= { "w_ranged_charge", 0.0f };
	myAnimations[PlayerAnimationState::RangeChargeIdle]	= { "w_ranged_charge_idle", 0.0f };
	myAnimations[PlayerAnimationState::Upgrade]			= { "w_upgrade", 0.0f };
	myAnimations[PlayerAnimationState::FusePickup]		= { "w_fuse_pickup", 0.0f };
	myAnimations[PlayerAnimationState::AntennaPlace]	= { "w_antenna_place", 0.0f };
	myAnimations[PlayerAnimationState::Death]			= { "w_death", 0.0f };
}
void PlayerAnimationComponent::UpdateBlendWeights(float aDeltaTime)
{
	float blendStep = myBlendSpeed * aDeltaTime;

	for (auto& [state, animation] : myAnimations)
	{
		float targetWeight = (state == myCurrentState) ? 1.0f : 0.0f;

		if (animation.Weight < targetWeight)
		{
			animation.Weight = std::min(animation.Weight + blendStep, targetWeight);
		}
		else
		{
			animation.Weight = std::max(animation.Weight - blendStep, targetWeight);
		}
		myGraph->SetFloatParameter(animation.Name, animation.Weight);
	}

}
