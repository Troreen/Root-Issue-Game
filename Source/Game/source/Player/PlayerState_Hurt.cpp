#include "PlayerState_Hurt.h"

void PlayerState_Hurt::Update(float aDeltaTime, PlayerControllerComponent& aController)
{
	myHurtTimer;
	aController;
	aDeltaTime;
}

void PlayerState_Hurt::SetValues()
{
	if (myAnimationGraph)
	{
		myAnimationGraph->SetFloatParameter("w_hurt", 1.0f);
	}

	myHurtTimer = 0.5f;
}

void PlayerState_Hurt::ResetValues()
{
	if (myAnimationGraph)
	{
		myAnimationGraph->SetFloatParameter("w_hurt", 0.0f);
	}
}
