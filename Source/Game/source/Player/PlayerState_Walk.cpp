#include "PlayerState_Walk.h"
#include <CommonUtilities/Transform.hpp>
#include <CommonUtilities/AABB3D.hpp>
#include "Essentials/Essentials.h"
#include "GameObject.h"
#include "PlayerState_Master.h"
#include "PlayerControllerComponent.h"
#include "BulletComponent.h"
#include <memory>
#include "AnimationGraphComponent.h"
#include "GameObjectFactory.h"
#include "SceneObjectData.h"
#include "DamageableComponent.h"


PlayerState_Walk::PlayerState_Walk()
{
	myWalkSpeed = 600.f;
}

void PlayerState_Walk::Update(float aDeltaTime, PlayerControllerComponent& aController)
{
	GameObject* player = aController.GetOwner();
	if (!player)
	{
		return;
	}

	CommonUtilities::Vector3<float> direction = myInput->GetFacingDirection();

	float myWalkAnimation = 0.f;

	if (myInput->IsWalking())
	{	
		direction *= myWalkSpeed * aDeltaTime;
		myWalkAnimation = 1.f;
		player->GetTransform().Translate(direction);
		player->GetTransform().SetYawPitchRollRadians({ std::atan2f(direction.x, direction.z), 0, 0 });
	}

	if (Essentials::globalInputManager.get()->PressingPlayerAim() && myHasGun)
	{
		myWalkAnimation = 0;
		aController.SetState(PlayerState_Master::Instance().myChargeAttackState.get());
		return;
	}
	else if (Essentials::globalInputManager.get()->PressingPlayerAttack())
	{
		myWalkAnimation = 0;
		aController.SetState(PlayerState_Master::Instance().myAttackState.get());
		return;
	}
	if (myAnimationGraph)
	{
		if (myWalkAnimation > 0)
		{
			myPlayerAnimation->BlendTo(PlayerAnimationState::Walk, 20);
		}
		else
		{
			myPlayerAnimation->BlendTo(PlayerAnimationState::None, 20);
		}
		//myAnimationGraph->SetFloatParameter("w_walk", myWalkAnimation);
	}
}

void PlayerState_Walk::SetHasGun(bool aGiveGun)
{
	myOwner->GetComponent<PlayerControllerComponent>()->SetState(PlayerState_Master::Instance().myUpgradeState.get());
	myHasGun = aGiveGun;
}

void PlayerState_Walk::SetHasGunOnStart(bool aGiveGun)
{
	myHasGun = aGiveGun;
}

