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

	myAnimationGraph->SetFloatParameter("w_upgrade", 1.f);
}

void PlayerState_Pickup_Gun::ResetValues()
{
	myAnimationGraph->SetFloatParameter("w_upgrade", 0.f);
}
