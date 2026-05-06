#include "PlayerState_Charge_Attack.h"
#include "PlayerControllerComponent.h"
#include "PlayerState_Master.h"
#include "Essentials/Essentials.h"
#include "AnimationGraphComponent.h"
#include "BulletComponent.h"

void PlayerState_Charge_Attack::Update(float aDeltaTime, PlayerControllerComponent& aPlayerController)
{
	myChargeTimer -= aDeltaTime;

	if (myChargeTimer > 0)
	{
		aPlayerController.GetOwner()->GetComponent<AnimationGraphComponent>()->SetFloatParameter("w_ranged_charge", 1);
		aPlayerController.GetOwner()->GetComponent<AnimationGraphComponent>()->SetFloatParameter("w_ranged_charge_idle", 0);
	}
	else
	{
		aPlayerController.GetOwner()->GetComponent<AnimationGraphComponent>()->SetFloatParameter("w_ranged_charge", 0);
		aPlayerController.GetOwner()->GetComponent<AnimationGraphComponent>()->SetFloatParameter("w_ranged_charge_idle", 1);
	}


	if (Essentials::GetEssentials().globalInputManager->IsKeyReleased(static_cast<int>(Keys::SPACE)))
	{
		aPlayerController.GetOwner()->GetComponent<AnimationGraphComponent>()->SetFloatParameter("w_ranged_charge", 0);
		aPlayerController.GetOwner()->GetComponent<AnimationGraphComponent>()->SetFloatParameter("w_ranged_charge_idle", 0);

		if (myChargeTimer > 0)
		{
			aPlayerController.SetState(PlayerState_Master::Instance().myWalkState.get());
			return;
		}

		aPlayerController.GetOwner()->GetComponent<BulletComponent>()->SpawnBullet();
		aPlayerController.SetState(PlayerState_Master::Instance().myShootState.get());
	}
}

void PlayerState_Charge_Attack::ResetValues()
{
	myChargeTimer = 0.5f;
}
