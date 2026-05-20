#pragma once
#include "PlayerState.h"

class PlayerState_Attack : public PlayerState
{
public:

	PlayerState_Attack();

	void Update(float aDeltaTime, PlayerControllerComponent& aController) override;
	void SetValues() override;
private:

	bool myInputAttack;
	bool myAttackFromRight;
	bool myHasSpawnedHitbox;
	float myAttackTime;
	float myAttackTimer;
	float myAttackLungeImpulse;
	float myAttackLungeDamp;
	float myAttackLungeSpeed;
};
