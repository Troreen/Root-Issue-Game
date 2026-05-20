#pragma once
#include "PlayerState.h"

class PlayerState_Pickup_Gun : public PlayerState
{
public:

	void Update(float aDeltaTimer, PlayerControllerComponent& aController) override;
	void SetValues() override;
	void ResetValues() override;


private:

	float myPickupTimer;
};