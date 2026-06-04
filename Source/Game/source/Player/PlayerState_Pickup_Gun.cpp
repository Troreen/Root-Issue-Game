#include "PlayerState_Pickup_Gun.h"
#include "PlayerControllerComponent.h"
#include "PlayerState_Master.h"

void PlayerState_Pickup_Gun::Update(float aDeltaTimer, PlayerControllerComponent& aController)
{
	myPickupTimer -= aDeltaTimer;

	if (myPickupTimer < 0)
	{
		aController.SetState(PlayerState_Master::Instance().myWalkState.get());
	}
}

void PlayerState_Pickup_Gun::SetValues()
{
	myPickupTimer = 3.f;

	myPlayerAnimation->BlendTo(PlayerAnimationState::Upgrade, 20);
}

void PlayerState_Pickup_Gun::ResetValues()
{
	myPlayerAnimation->BlendTo(PlayerAnimationState::None, 20);
}
