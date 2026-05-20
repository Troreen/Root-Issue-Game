#pragma once
#include "PlayerState.h"
class PlayerState_Hurt : public PlayerState
{
public:

	void Update(float aDeltaTime, PlayerControllerComponent& aController) override;
	void SetValues() override;
	void ResetValues() override;

private:

	float myHurtTimer;
};