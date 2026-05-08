#include "PlayerState_Charge_Attack.h"
#include "PlayerState_Master.h"
#include "PlayerControllerComponent.h"

PlayerState_Charge_Attack::PlayerState_Charge_Attack()
{
	myBullet = nullptr;
	myMouse = nullptr;
}

void PlayerState_Charge_Attack::Update(float aDeltaTime, PlayerControllerComponent& aController)
{
	GameObject* owner = aController.GetOwner();
	if (!owner || !myAnimationGraph)
	{
		return;
	}

	myBullet = owner->GetComponent<BulletComponent>();
	myMouse = owner->GetComponent<MouseDirectionComponent>();
	if (!myBullet || !myMouse)
	{
		aController.SetState(PlayerState_Master::Instance().myWalkState.get());
		return;
	}

	owner->GetTransform().SetYawPitchRollRadians(std::atan2f(myMouse->GetWorldDirection().y, myMouse->GetWorldDirection().x), 0, 0);

	myChargeTimer -= aDeltaTime;

	if (myChargeTimer > 0)
	{
		myAnimationGraph->SetFloatParameter("w_ranged_charge", 1);
		myAnimationGraph->SetFloatParameter("w_ranged_charge_idle", 0);
	}
	else
	{
		myAnimationGraph->SetFloatParameter("w_ranged_charge", 0);
		myAnimationGraph->SetFloatParameter("w_ranged_charge_idle", 1);
	}


	if (Essentials::GetEssentials().globalInputManager->IsKeyReleased(static_cast<int>(Keys::MOUSELBUTTON)))
	{
		myAnimationGraph->SetFloatParameter("w_ranged_charge", 0);
		myAnimationGraph->SetFloatParameter("w_ranged_charge_idle", 0);

		if (myChargeTimer > 0)
		{
			aController.SetState(PlayerState_Master::Instance().myWalkState.get());
			return;
		}

		aController.FireBullet();
		aController.SetState(PlayerState_Master::Instance().myShootState.get());
	}
}

void PlayerState_Charge_Attack::ResetValues()
{
	myChargeTimer = 0.5f;
}
