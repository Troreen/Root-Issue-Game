#pragma once
#include "PlayerState.h"

class PlayerState_Shoot : public PlayerState
{
public:

	void Update(float aDeltaTime, PlayerControllerComponent& aController) override;
	void SetValues() override;

private:

	bool myFired;
	float myChargeTimer;
	float myFireTimer;
};