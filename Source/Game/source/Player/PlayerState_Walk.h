#pragma once
#include "PlayerState.h"
#include "GameObject.h"

class PlayerState_Walk : public PlayerState 
{
public:
	PlayerState_Walk();
	~PlayerState_Walk() = default;

	void Update(float aDeltaTime, PlayerControllerComponent& aController) override;
private:

	float myWalkSpeed;
	//Test
};

