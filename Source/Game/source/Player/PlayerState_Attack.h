#pragma once
#include "PlayerState.h"

class PlayerState_Attack : public PlayerState
{
public:

	PlayerState_Attack();

	void Update(float aDeltaTime, PlayerControllerComponent& aController) override;
	void SetValues() override;
	void ResetValues() override;

private:

	bool myEnd;
	bool myInputAttack;
	bool myAttackFromRight;
	bool myHasSpawnedHitbox;
	float myAttackTime;
	float myAttackTimer;
	float myNextAttackTime;
	float myAttackLungeImpulse;
	float myAttackLungeDamp;
	float myAttackLungeSpeed;
};
