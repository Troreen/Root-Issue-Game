#pragma once
#include "PlayerState.h"
#include "GameObject.h"

class PlayerState_Walk : public PlayerState 
{
public:
	PlayerState_Walk();
	~PlayerState_Walk() = default;

	void Update(float aDeltaTime, PlayerControllerComponent& aController) override;

	void SetHasGun(bool aGiveGun);
	void SetHasGunOnStart(bool aGiveGun);
private:
	
	bool myHasGun;
	float myWalkSpeed;
};

