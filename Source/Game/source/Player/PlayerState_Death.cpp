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

void PlayerState_Death::SetValues()
{
	if (myAnimationGraph)
	{
		if (myAnimationGraph)
		{
			myPlayerAnimation->BlendTo(PlayerAnimationState::Death, 20);
		}
	}
	myDeathTimer = 3.f;
}

void PlayerState_Death::ResetValues()
{
	if (myAnimationGraph)
	{
		if (myAnimationGraph)
		{
			myPlayerAnimation->BlendTo(PlayerAnimationState::None, 20);
		}
	}
}


