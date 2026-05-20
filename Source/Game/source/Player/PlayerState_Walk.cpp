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
	CommonUtilities::Vector3<float> direction;
	CommonUtilities::Vector3<float> forwardAxis = Essentials::globalCamera.get()->GetCamera().GetTransform().GetForward();
	forwardAxis.y = 0;
	forwardAxis.Normalize();
	CommonUtilities::Vector3<float> rightAxis = Essentials::globalCamera.get()->GetCamera().GetTransform().GetRight();

	float myWalkAnimation = 0.f;

	if (Essentials::globalInputManager.get()->IsKeyHeld(static_cast<int>(Keys::W)))
	{
		direction += forwardAxis;
	}
	if (Essentials::globalInputManager.get()->IsKeyHeld(static_cast<int>(Keys::A)))
	{
		direction -= rightAxis;
	}
	if (Essentials::globalInputManager.get()->IsKeyHeld(static_cast<int>(Keys::S)))
	{
		direction -= forwardAxis;
	}
	if (Essentials::globalInputManager.get()->IsKeyHeld(static_cast<int>(Keys::D)))
	{
		direction += rightAxis;
	}

	GameObject* player = aController.GetOwner();
	if (!player)
	{
		return;
	}

	direction = direction.GetNormalized() * myWalkSpeed * aDeltaTime;

	if (direction.LengthSqr() > 0)
	{	
		myWalkAnimation = 1.f;
		player->GetTransform().Translate(direction);
		player->GetTransform().SetYawPitchRollRadians({ std::atan2f(direction.x, direction.z), 0, 0 });
	}

	if (Essentials::globalInputManager.get()->IsKeyHeld(static_cast<int>(Keys::RETURN)))
	{
		aController.SetState(PlayerState_Master::Instance().myDeathState.get());
		myWalkAnimation = 0.f;
	}

	if (Essentials::globalInputManager.get()->IsKeyHeld(static_cast<int>(Keys::MOUSELBUTTON)) && myHasGun)
	{
		myWalkAnimation = 0;
		aController.SetState(PlayerState_Master::Instance().myChargeAttackState.get());
	}
	else if (Essentials::globalInputManager.get()->IsKeyPressed(static_cast<int>(Keys::SPACE)))
	{
		myWalkAnimation = 0;
		aController.SetState(PlayerState_Master::Instance().myAttackState.get());
	}
	if (myAnimationGraph)
	{
		myAnimationGraph->SetFloatParameter("w_walk", myWalkAnimation);
	}

}

void PlayerState_Walk::SetHasGun(bool aGiveGun)
{
	if (GameObject* player = myOwner)
	{
		player->GetComponent<PlayerControllerComponent>()->SetState(PlayerState_Master::Instance().myUpgradeState.get());
	}
	myHasGun = aGiveGun;
}

