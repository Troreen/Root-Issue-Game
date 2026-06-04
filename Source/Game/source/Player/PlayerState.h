#pragma once
#include "GameObject.h"
#include "Essentials/Essentials.h"
#include "AnimationGraphComponent.h"
#include "InputComponent.h"
#include "ParticleEmitterComponent.h"
#include "PlayerAnimationComponent.h"
#include "StaticSpriteComponent.h"

class PlayerControllerComponent;

class PlayerState
{
public:
	PlayerState()
	{
	}
	virtual void Update(float aTimeDelta, PlayerControllerComponent& aController) = 0;
	virtual void SetValues() {};
	virtual void ResetValues() {};
	void BindToOwner(GameObject* anOwner)
	{
		myOwner = anOwner;
		myAnimationGraph = myOwner ? myOwner->GetComponent<AnimationGraphComponent>() : nullptr;
		myInput = myOwner ? myOwner->GetComponent<InputComponent>() : nullptr;
		myEmitter = myOwner ? myOwner->GetComponent<ParticleEmitterComponent>() : nullptr;
		myPlayerAnimation = myOwner ? myOwner->GetComponent<PlayerAnimationComponent>() : nullptr;
		mySprite = myOwner ? myOwner->GetComponent<StaticSpriteComponent>() : nullptr;
	}
protected:

	GameObject* myOwner = nullptr;
	AnimationGraphComponent* myAnimationGraph = nullptr;
	InputComponent* myInput = nullptr;
	ParticleEmitterComponent* myEmitter = nullptr;
	PlayerAnimationComponent* myPlayerAnimation = nullptr;
	StaticSpriteComponent* mySprite = nullptr;

private:
};
