#include "PlayerState_Attack.h"
#include "PlayerControllerComponent.h"
#include "PlayerState_Master.h"
#include "AnimationGraphComponent.h"
#include "CombatSystem.h"
#include "GameObject.h"
#include "Essentials/Essentials.h"
#include "CommonUtilities/Transform.hpp"

namespace
{
	void StartPlayerMeleeAttack(GameObject& aPlayer)
	{
		AttackData attack;
		attack.owner = &aPlayer;
		attack.team = CombatTeam::Player;
		attack.type = AttackType::MeleeLight;
		attack.collisionShape = CollisionShapeType::Sphere;
		attack.damage = 1;
		attack.localCenterOffset = CommonUtilities::Vector3<float>(0.0f, 90.0f, 0.0f);
		attack.radius = 190.0f;
		attack.activeDurationSeconds = 0.16f;
		attack.knockbackStrength = 450.0f;
		attack.onlyHitForwardHemisphere = true;
		attack.targetLayers.AddLayer(ObjectLayer::Enemy);

		CombatService::StartAttack(attack);
	}
}

PlayerState_Attack::PlayerState_Attack()
{
	myAttackFromRight = false;
	myHasSpawnedHitbox = false;

	myAttackLungeImpulse = 10.f;
	myAttackTime = 0.4f;

	myAttackLungeDamp = myAttackLungeImpulse / myAttackTime;
}

void PlayerState_Attack::Update(float aDeltaTime, PlayerControllerComponent& aPlayerController)
{
	if (!myHasSpawnedHitbox)
	{
		StartPlayerMeleeAttack(*aPlayerController.GetOwner());
		myHasSpawnedHitbox = true;
	}

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
			myHasSpawnedHitbox = false;
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
	myHasSpawnedHitbox = false;
	myAttackTimer = myAttackTime;
	myAttackLungeSpeed = myAttackLungeImpulse;
}
