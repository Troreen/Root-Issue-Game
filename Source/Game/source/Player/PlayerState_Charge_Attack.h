#pragma once
#include "PlayerState.h"

class PlayerState_Charge_Attack : public PlayerState
{
public:

	void Update(float aDeltaTime, PlayerControllerComponent& aPlayerController) override;
	void ResetValues() override;

private:
	float myChargeTimer;
};