#include "PlayerState_Death.h"
#include "PlayerControllerComponent.h"
#include "CommonUtilities/Quaternion.hpp"

PlayerState_Death::PlayerState_Death()
{
}

void PlayerState_Death::Update(float aTimeDelta, PlayerControllerComponent& aController)
{
	myDeathTimer -= aTimeDelta;

	if (myDeathTimer < 0)
	{
		aController;

		Essentials::globalPostMaster->SendMsg({ MessageType::ReloadScene });
	}
}

void PlayerState_Death::ResetValues()
{
	myDeathTimer = 3.f;
	if (myAnimationGraph)
	{
		myAnimationGraph->SetFloatParameter("w_death", 1.f);
	}
}
