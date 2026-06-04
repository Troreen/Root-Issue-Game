#include "PlayerState_Shoot.h"
#include "Essentials/Essentials.h"
#include "PlayerControllerComponent.h"
#include "PlayerState_Master.h"
#include "AnimationGraphComponent.h"
#include "GameObject.h"

void PlayerState_Shoot::Update(float aDeltaTime, PlayerControllerComponent& aController)
{
	if (!myAnimationGraph)
	{
		aController.SetState(PlayerState_Master::Instance().myWalkState.get());
		return;
	}

	myFireTimer -= aDeltaTime;
	myPlayerAnimation->BlendTo(PlayerAnimationState::RangeAttack, 20);

	if (myFireTimer < 0)
	{
		if (aController.IsMoveInput())
		{
			myPlayerAnimation->BlendTo(PlayerAnimationState::None, 20);
			aController.SetState(PlayerState_Master::Instance().myWalkState.get());
		}
	}
}

void PlayerState_Shoot::SetValues()
{
	myFireTimer = 0.5f;
}
