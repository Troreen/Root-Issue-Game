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
	}
	virtual void Update(float aTimeDelta, PlayerControllerComponent& aController) = 0;
	virtual void ResetValues() {};
	void BindToOwner(GameObject* anOwner)
	{
		myOwner = anOwner;
		myAnimationGraph = myOwner ? myOwner->GetComponent<AnimationGraphComponent>() : nullptr;
	}
protected:

	GameObject* myOwner = nullptr;
	AnimationGraphComponent* myAnimationGraph = nullptr;

private:
};
