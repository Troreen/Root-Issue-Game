#include "PlayerState_Attack.h"
#include "PlayerControllerComponent.h"
#include "PlayerState_Master.h"
#include "AnimationGraphComponent.h"
#include "GameObject.h"
#include "Essentials/Essentials.h"
#include "CommonUtilities/Transform.hpp"

PlayerState_Attack::PlayerState_Attack()
{
	myAttackFromRight = false;

	myAttackLungeImpulse = 10.f;
	myAttackTime = 0.4f;

	myAttackLungeDamp = myAttackLungeImpulse / myAttackTime;
}

void PlayerState_Attack::Update(float aDeltaTime, PlayerControllerComponent& aPlayerController)
{

	if (myAttackTimer < 0.2f && Essentials::GetEssentials().globalInputManager->IsKeyPressed(static_cast<int>(Keys::RETURN)))
	{
		myInputAttack = true;
	}

	myAttackLungeSpeed -= aDeltaTime * myAttackLungeDamp;

	CommonUtilities::Transform<float>& transform = aPlayerController.GetOwner()->GetTransform();

	transform.Translate(transform.GetForward() * std::max(myAttackLungeSpeed, 0.f));

	if (myAttackTimer < 0.1f)
	{
		if (myAttackTimer < 0)
		{
			aPlayerController.SetState(PlayerState_Master::Instance().myWalkState.get());

			aPlayerController.GetOwner()->GetComponent<AnimationGraphComponent>()->SetFloatParameter("w_attack_basic01", 0);
			aPlayerController.GetOwner()->GetComponent<AnimationGraphComponent>()->SetFloatParameter("w_attack_basic02", 0);

			return;
		}

		if (myInputAttack)
		{
			myInputAttack = false;
			myAttackTimer = myAttackTime;
			myAttackFromRight = !myAttackFromRight;
			myAttackLungeSpeed = myAttackLungeImpulse;
		}
	}



	myAttackTimer -= aDeltaTime;
	aPlayerController.GetOwner()->GetComponent<AnimationGraphComponent>()->SetFloatParameter("w_attack_basic01", myAttackFromRight ? 1.f : 0.f);
	aPlayerController.GetOwner()->GetComponent<AnimationGraphComponent>()->SetFloatParameter("w_attack_basic02", myAttackFromRight ? 0.f : 1.f);
}

void PlayerState_Attack::ResetValues()
{
	myInputAttack = false;
	myAttackFromRight = !myAttackFromRight;
	myAttackTimer = myAttackTime;
	myAttackLungeSpeed = myAttackLungeImpulse;
}
