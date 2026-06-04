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
	Tga::Vector2f direction;

	if (myInput->IsCharging())
	{
		direction.x = myInput->GetAimingDirection().x;
		direction.y = myInput->GetAimingDirection().z;
	}
	else
	{
		direction = myMouse->GetWorldDirection();
	}

	if (held)
	{
		owner->GetTransform().SetYawPitchRollRadians(std::atan2f(direction.x, direction.y), 0, 0);
	}

	myChargeTimer -= aDeltaTime;

	if (myChargeTimer > 0)
	{
		if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eCharge))
		{
			Essentials::globalAudioManager->PlaySFX(SoundID::eCharge);
		}
		myPlayerAnimation->BlendTo(PlayerAnimationState::RangeCharge, 20);

	}
	else
	{
		myEmitter->SetEnabled(true);
		myPlayerAnimation->BlendTo(PlayerAnimationState::RangeChargeIdle, 100);
	}

	myEmitter->SetOffset(ParticleType::EnergySmall, { -15, 75, 120 });
	myEmitter->SetEmissionDirection(ParticleType::EnergySmall, myOwner->GetTransform().GetForward());

	if (!held)
	{
		myPlayerAnimation->BlendTo(PlayerAnimationState::None, 20);

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

	Essentials::myCursor->SetCursorVisible(Essentials::globalInputManager->IsKeyHeld(static_cast<int>(Keys::MOUSELBUTTON)));

	mySprite->SetEnabled(true);
}

void PlayerState_Charge_Attack::ResetValues()
{
	myEmitter->SetEnabled(false);
	Essentials::myCursor->SetCursorVisible(false);

	mySprite->SetEnabled(false);
}
