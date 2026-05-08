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
	myAnimationGraph->SetFloatParameter("w_ranged_attack", 1);

	if (myFireTimer < 0)
	{
		myAnimationGraph->SetFloatParameter("w_ranged_attack", 0);
		aController.SetState(PlayerState_Master::Instance().myWalkState.get());
	}
}

void PlayerState_Shoot::ResetValues()
{
	myFireTimer = 0.5f;
}
