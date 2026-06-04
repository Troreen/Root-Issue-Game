#pragma once
#include "PlayerState.h"
#include "BulletComponent.h"
#include "MouseDirectionComponent.h"

class PlayerState_Charge_Attack : public PlayerState
{
public:

	PlayerState_Charge_Attack();

	void Update(float aDeltaTime, PlayerControllerComponent& aController) override;
	void SetValues() override;
	void ResetValues() override;
private:
	float myChargeTimer;
	BulletComponent* myBullet;
	MouseDirectionComponent* myMouse;
};