#include "PlayerState_Charge_Attack.h"
#include "PlayerState_Master.h"
#include "PlayerControllerComponent.h"
#include "Essentials.h"

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

	myMouse = owner->GetComponent<MouseDirectionComponent>();
	if (!myMouse)
	{
		aController.SetState(PlayerState_Master::Instance().myWalkState.get());
		return;
	}

	bool held = Essentials::GetEssentials().globalInputManager->PressingPlayerAim();
	Tga::Vector2f direction = Essentials::globalInputManager->RightStickHeldDown() ? Essentials::globalInputManager->RightStick() : myMouse->GetWorldDirection();

	if (held)
	{
		owner->GetTransform().SetYawPitchRollRadians(std::atan2f(myMouse->GetWorldDirection().y, myMouse->GetWorldDirection().x), 0, 0);
	}

	myChargeTimer -= aDeltaTime;

	if (myChargeTimer > 0)
	{
		if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eCharge))
		{
			Essentials::globalAudioManager->PlaySFX(SoundID::eCharge);
		}
		myAnimationGraph->SetFloatParameter("w_ranged_charge", 1);
		myAnimationGraph->SetFloatParameter("w_ranged_charge_idle", 0);
	}
	else
	{
		myAnimationGraph->SetFloatParameter("w_ranged_charge", 0);
		myAnimationGraph->SetFloatParameter("w_ranged_charge_idle", 1);
	}


	if (!held)
	{
		myAnimationGraph->SetFloatParameter("w_ranged_charge", 0);
		myAnimationGraph->SetFloatParameter("w_ranged_charge_idle", 0);
		Essentials::globalAudioManager->StopMusic(SoundID::eCharge, true);

		if (myChargeTimer > 0)
		{
			aController.SetState(PlayerState_Master::Instance().myWalkState.get());
			return;
		}
		if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eShoot))
		{
			Essentials::globalAudioManager->PlaySFX(SoundID::eShoot);
		}
		aController.FireBullet();
		aController.SetState(PlayerState_Master::Instance().myShootState.get());
	}
}

void PlayerState_Charge_Attack::SetValues()
{
	myChargeTimer = 0.5f;
}
