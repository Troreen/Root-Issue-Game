#include "PlayerState_Death.h"
#include "PlayerControllerComponent.h"
#include "CommonUtilities/Quaternion.hpp"

PlayerState_Death::PlayerState_Death()
{
	myDeathTimer = 3.f;
}

void PlayerState_Death::Update(float aTimeDelta, PlayerControllerComponent& aController)
{
	myDeathTimer -= aTimeDelta;

	if (myDeathTimer < 0)
	{
		aController;
	}
}

void PlayerState_Death::ResetValues()
{
	myAnimationGraph->SetFloatParameter("w_death", 1.f);
}
