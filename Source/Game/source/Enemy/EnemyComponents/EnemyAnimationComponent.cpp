#include "EnemyAnimationComponent.h"
#include "AnimationGraphComponent.h"
#include "GameObject.h"

//EnemyAnimationComponent::EnemyAnimationComponent(const EnemyData& /*someData*/)
//{
//}

void EnemyAnimationComponent::OnStart()
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

void EnemyAnimationComponent::OnUpdate(float aDeltaTime)
{
	if (!myGraph)
	{
		return;
	}

	UpdateBlendWeights(aDeltaTime);
}

void EnemyAnimationComponent::BlendTo(EnemyAnimationState aState, float aBlendSpeed)
{
	myCurrentState = aState;
	myBlendSpeed = aBlendSpeed;
}

void EnemyAnimationComponent::RegisterAnimations()
{
	myAnimations[EnemyAnimationState::Spawn]         = { "w_spawn", 0.0f };
	myAnimations[EnemyAnimationState::Idle]          = { "w_idle", 0.0f };
	myAnimations[EnemyAnimationState::IdleAggro]     = { "w_idle_aggro", 0.0f };
	myAnimations[EnemyAnimationState::Aggro]         = { "w_aggro", 0.0f };
	myAnimations[EnemyAnimationState::Walk]          = { "w_walk", 0.0f };
	myAnimations[EnemyAnimationState::AggroWalk]     = { "w_aggro_walk", 0.0f };
	myAnimations[EnemyAnimationState::Attack]        = { "w_attack" , 0.0f };
	myAnimations[EnemyAnimationState::ChargeAttack]  = { "w_charge", 0.0f };
	myAnimations[EnemyAnimationState::RollIdle]      = { "w_roll_idle", 0.0f };
	myAnimations[EnemyAnimationState::Hurt]          = { "w_hurt", 0.0f };
	myAnimations[EnemyAnimationState::Block]         = { "w_block", 0.0f };
	myAnimations[EnemyAnimationState::KnockDown]     = { "w_knockback", 0.0f };
	myAnimations[EnemyAnimationState::Death]         = { "w_death", 0.0f };
	myAnimations[EnemyAnimationState::DeathLaying]   = { "w_death_laying", 0.0f };
	myAnimations[EnemyAnimationState::DeathStanding] = { "w_death_standing", 0.0f};
}
void EnemyAnimationComponent::UpdateBlendWeights(float aDeltaTime)
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
