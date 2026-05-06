#pragma once
#include "GameObject.h"
#include "Essentials/Essentials.h"
#include "AnimationGraphComponent.h"

class PlayerControllerComponent;

class PlayerState
{
public:
	PlayerState()
	{
		myOwner = Essentials::GetPlayer();
		myAnimationGraph = myOwner->GetComponent<AnimationGraphComponent>();
	}
	virtual void Update(float aTimeDelta, PlayerControllerComponent& aController) = 0;
	virtual void ResetValues() {};
protected:

	GameObject* myOwner;
	AnimationGraphComponent* myAnimationGraph;

private:
};