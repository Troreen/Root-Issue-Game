#include "PlayerState_Hurt.h"
#include "PlayerControllerComponent.h"
#include "PlayerState_Master.h"
#include "KnockbackComponent.h"

void PlayerState_Hurt::Update(float aDeltaTime, PlayerControllerComponent& aController)
{
	myHurtTimer;
	aController;
	aDeltaTime;

	myHurtTimer -= aDeltaTime;

	if (myHurtTimer < 0)
	{
		aController.SetState(PlayerState_Master::Instance().myWalkState.get());
	}
}

void PlayerState_Hurt::SetValues()
{
	if (myAnimationGraph)
	{
		myPlayerAnimation->BlendTo(PlayerAnimationState::Hurt, 20);
	}

	auto& transform = myOwner->GetTransform();
	auto direction = myOwner->GetComponent<KnockbackComponent>()->GetDirection() * -1.f;

	transform.SetYawPitchRollRadians({ std::atan2f(direction.x, direction.z), 0, 0 });

	myHurtTimer = 0.5f;
}

void PlayerState_Hurt::ResetValues()
{
	if (myAnimationGraph)
	{
		myPlayerAnimation->BlendTo(PlayerAnimationState::None, 20);
	}
}
