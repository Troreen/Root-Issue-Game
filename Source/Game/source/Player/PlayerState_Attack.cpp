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
		attack.targetLayers.AddLayer(ObjectLayer::Switch);
		attack.targetLayers.AddLayer(ObjectLayer::WorldDamageable);

		CombatService::StartAttack(attack);
	}
}

PlayerState_Attack::PlayerState_Attack()
{
	//
	myAttackFromRight = false;
	myHasSpawnedHitbox = false;

	myAttackLungeImpulse = 10.f;

	myAttackTime = 1.1f;
	myNextAttackTime = myAttackTime - 0.35f;

	myAttackLungeDamp = myAttackLungeImpulse / 0.4f;
}

void PlayerState_Attack::Update(float aDeltaTime, PlayerControllerComponent& aController)
{
	if (!myOwner || !myAnimationGraph)
	{
		aController.SetState(PlayerState_Master::Instance().myWalkState.get());
		return;
	}

	if (myEnd)
	{
		aController.SetState(PlayerState_Master::Instance().myWalkState.get());
		return;
	}

	if (!myHasSpawnedHitbox)
	{
		// Animation-event combat test path: attack audio and hitbox are now
		// authored in the attack animation event script.
		// Essentials::globalAudioManager->PlaySFX(SoundID::ePlayerAttack);
		//StartPlayerMeleeAttack(*myOwner);
		myHasSpawnedHitbox = true;
	}

	if (myAttackTimer < myNextAttackTime + 0.2f && Essentials::GetEssentials().globalInputManager->PressingPlayerAttack())
	{
		myInputAttack = true;
	}

	myAttackLungeSpeed -= aDeltaTime * myAttackLungeDamp;

	CommonUtilities::Transform<float>& transform = myOwner->GetTransform();

	transform.Translate(transform.GetForward() * std::max(myAttackLungeSpeed, 0.f));

	if (myAttackTimer < myNextAttackTime)
	{
		if (myInputAttack)
		{
			myInputAttack = false;
			myAttackTimer = myAttackTime;
			myAttackFromRight = !myAttackFromRight;
			myAttackLungeSpeed = myAttackLungeImpulse;
			myHasSpawnedHitbox = false;
			if (myAttackFromRight)
			{
				myPlayerAnimation->BlendTo(PlayerAnimationState::Attack2, 20);
			}
			else
			{
				myPlayerAnimation->BlendTo(PlayerAnimationState::Attack3, 20);
			}

		}

		else if (aController.IsMoveInput())
		{
			aController.SetState(PlayerState_Master::Instance().myWalkState.get());
		}
	}

	myAttackTimer -= aDeltaTime;
}

void PlayerState_Attack::SetValues()
{
	//myAnimationGraph->SetFloatParameter("w_attack_basic01", 1.f);

	myPlayerAnimation->BlendTo(PlayerAnimationState::Attack1, 20);

	myEnd = false;
	myInputAttack = false;
	myAttackFromRight = false;
	myHasSpawnedHitbox = false;
	myAttackTimer = myAttackTime;
	myAttackLungeSpeed = myAttackLungeImpulse;
}

void PlayerState_Attack::ResetValues()
{
	//myAnimationGraph->SetFloatParameter("w_attack_basic01", 0);
	//myAnimationGraph->SetFloatParameter("w_attack_basic02", 0);
	//myAnimationGraph->SetFloatParameter("w_attack_basic03", 0);
	myPlayerAnimation->BlendTo(PlayerAnimationState::None);

}