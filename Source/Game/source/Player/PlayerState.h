#pragma once

class PlayerControllerComponent;

class PlayerState
{
public:

	virtual void Update(float aTimeDelta, PlayerControllerComponent& aPlayerController) = 0;
	virtual void ResetValues() {};
private:
};