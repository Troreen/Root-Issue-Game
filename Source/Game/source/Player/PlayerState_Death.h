#pragma once
#include "PlayerState.h"

class PlayerState_Death : public PlayerState
{
public:
	PlayerState_Death();

    void Update(float aTimeDelta, PlayerControllerComponent& aController) override;
	void SetValues() override;
	void ResetValues() override;

private:

	float myDeathTimer;
};