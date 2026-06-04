#include "RollingEnemyAIComponent.h"

#include "EnemyMovementComponent.h"
#include "EnemyTargetingComponent.h"
#include "EnemyAnimationComponent.h"
#include "ParticleEmitterComponent.h"
#include "EnemyAttackComponent.h"
#include "DamageableComponent.h"
#include "SphereColliderComponent.h"
#include "GameObject.h"
#include "Essentials.h"

RollingEnemyAIComponent::RollingEnemyAIComponent(const EnemyData& someEnemyData) : EnemyAIComponent(someEnemyData)
{
}

void RollingEnemyAIComponent::OnUpdate(float aDeltaTime)
{

	if (myCurrentState != EnemyState::Death &&
		myDamageableComponent->IsDead())
	{
		myAnimation->BlendTo(EnemyAnimationState::DeathStanding);
		ChangeState(EnemyState::Death);
		return;
	}

	switch (myCurrentState)
	{
	case EnemyState::Spawn:
		UpdateSpawn(aDeltaTime);
		break;

	case EnemyState::Idle:
		UpdateIdle(aDeltaTime);
		break;

	case EnemyState::Wander:
		UpdateWander(aDeltaTime);
		break;

	case EnemyState::React:

		UpdateReact(aDeltaTime);
		break;

	case EnemyState::Attacking:
	{
		if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eRoll))
		{
			Essentials::globalAudioManager->PlaySFX(SoundID::eRoll);
		}

		UpdateAttacking(aDeltaTime);
		break;
	}

	case EnemyState::Hurt:
	{
		if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eHeavyVox))
		{
			Essentials::globalAudioManager->PlaySFX(SoundID::eHeavyVox);
		}

		UpdateHurt(aDeltaTime);
		break;
	}
	case EnemyState::Stunned:
	{
		UpdateStunned(aDeltaTime);
		break;
	}

	case EnemyState::Death:
	{
		if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eEnemyDeadVox))
		{
			Essentials::globalAudioManager->PlaySFX(SoundID::eEnemyDeadVox);
		}

		UpdateDeath(aDeltaTime);
		break;
	}

	case EnemyState::ReturnHome:
		UpdateReturnHome(aDeltaTime);
		break;

	default:
		break;
	}
}

void RollingEnemyAIComponent::EnterStunnedState()
{
	StopRollingSound();

	myAttack->FinishRollingAttack();

	myMovement->StopMoving();

	myEmitterComponent->Burst(ParticleType::Test);

	myAnimation->BlendTo(EnemyAnimationState::KnockDown, 10.0f);

	myStunnedTimer = myEnemyData.StunTime;

	ChangeState(EnemyState::Stunned);
}

void RollingEnemyAIComponent::StopRollingSound()
{
	if (Essentials::globalAudioManager->IsEventPlaying(SoundID::eRoll))
	{
		Essentials::globalAudioManager->StopMusic(SoundID::eRoll, true);
	}
}

void RollingEnemyAIComponent::UpdateSpawn(float aDeltaTime)
{
	mySpawnTimer -= aDeltaTime;

	if (mySpawnTimer < 0.0f)
	{
		myAnimation->BlendTo(EnemyAnimationState::Walk);
		ChangeState(EnemyState::React);
	}
}

void RollingEnemyAIComponent::UpdateIdle(float aDeltaTime)
{
	myAnimation->BlendTo(EnemyAnimationState::Idle);

	myMovement->StopMoving();

	PickNewDirection();

	if (myTargeting->IsTargetInRange())
	{
		if (!myIsAggro)
		{
			myIsAggro = true;
			myReactionTimer = 0.6f;
			myAnimation->BlendTo(EnemyAnimationState::Aggro, 15.0f);
			ChangeState(EnemyState::React);
			return;
		}
		else
		{
			myAnimation->BlendTo(EnemyAnimationState::ChargeAttack, 15.0f);
			myAttack->StartAttack(myTargeting->GetTarget());
			ChangeState(EnemyState::Attacking);
			return;
		}
	}

	if (myDamageableComponent->TookDamageThisFrame())
	{
		myAnimation->BlendTo(EnemyAnimationState::Hurt, 15.0f);
		ChangeState(EnemyState::Hurt);
		return;
	}

	myIdleTimer -= aDeltaTime;

	if (myIdleTimer <= 0.0f)
	{
		myAnimation->BlendTo(EnemyAnimationState::Walk, 3.0f);
		ChangeState(EnemyState::Wander);
		myWanderTimer = GetRandomFloat(1.5f, 3.0f);
	}
}

void RollingEnemyAIComponent::UpdateWander(float aDeltaTime)
{
	myMovement->RotateTowards(myWanderDirection, aDeltaTime);
	myMovement->MoveForward(aDeltaTime);

	myWanderTimer -= aDeltaTime;

	if (myDamageableComponent->TookDamageThisFrame())
	{
		myAnimation->BlendTo(EnemyAnimationState::Hurt, 15.0f);
		ChangeState(EnemyState::Hurt);
		return;
	}

	if (myTargeting->IsTargetInRange())
	{
		if (!myIsAggro)
		{
			myIsAggro = true;
			myReactionTimer = 0.6f;
			myAnimation->BlendTo(EnemyAnimationState::Aggro, 15.0f);
			ChangeState(EnemyState::React);
			return;
		}
		else
		{
			myAnimation->BlendTo(EnemyAnimationState::ChargeAttack, 15.0f);
			myAttack->StartAttack(myTargeting->GetTarget());
			ChangeState(EnemyState::Attacking);
			return;
		}
	}

	if (myWanderTimer <= 0.0f)
	{
		if (GetRandomFloat(0.0f, 1.0f) < 0.3f)
		{
			myAnimation->BlendTo(EnemyAnimationState::Idle);
			ChangeState(EnemyState::Idle);
			myIdleTimer = GetRandomFloat(1.0f, 2.0f);
		}
		else
		{
			myWanderTimer = GetRandomFloat(1.5f, 3.0f);
			PickNewDirection();
		}
	}
}

void RollingEnemyAIComponent::UpdateReact(float aDeltaTime)
{
	GameObject* target = myTargeting->GetTarget();

	if (!target)
	{
		myAnimation->BlendTo(EnemyAnimationState::Idle);
		ChangeState(EnemyState::Idle);
		return;
	}

	Vector3f direction = target->GetTransform().GetPosition() - GetOwner()->GetTransform().GetPosition();

	direction.y = 0.f;
	direction.Normalize();

	myMovement->StopMoving();
	myMovement->RotateTowards(direction, aDeltaTime);


	myReactionTimer -= aDeltaTime;

	if (myReactionTimer <= 0.0f)
	{
		myAnimation->BlendTo(EnemyAnimationState::ChargeAttack);
		myAttack->StartAttack(target);
		ChangeState(EnemyState::Attacking);
		return;
	}
}

void RollingEnemyAIComponent::UpdateAttacking(float)
{
	if (myAttack->IsRollingActive())
	{
		if (!myIsRollingAnimActive)
		{
			myAnimation->BlendTo(EnemyAnimationState::RollIdle, 10.0f);
			myIsRollingAnimActive = true;
		}

		myMovement->SetMovementSpeed(myEnemyData.RollSpeed);
	}
	else
	{
		myIsRollingAnimActive = false;
	}

	if (myAttack->DidRollHitPlayerThisFrame())
	{
		EnterStunnedState();
		return;
	}

	if (myAttack->ConsumeWallHit())
	{
		EnterStunnedState();
		return;
	}

	if (myDamageableComponent->TookDamageThisFrame())
	{
		Vector3f emissionDir = GetOwner()->GetTransform().GetPosition() - Essentials::GetPlayer()->GetTransform().GetPosition();

		myEmitterComponent->SetEmissionDirection(ParticleType::Blood, emissionDir);

		myEmitterComponent->Burst(ParticleType::Blood);
	}

	if (!myAttack->IsAttacking())
	{
		StopRollingSound();

		myIsRollingAnimActive = false;

		const float distanceFromHome = (GetOwner()->GetTransform().GetPosition() - myStartPosition).Length();

		if (distanceFromHome > myLeashDistance && !myTargeting->IsTargetInRange())
		{
			myMovement->SetMovementSpeed(myEnemyData.WalkSpeed);
			myAnimation->BlendTo(EnemyAnimationState::Walk);
			ChangeState(EnemyState::ReturnHome);
			return;
		}

		if (myTargeting->IsTargetInRange())
		{
			myAttack->StartAttack(myTargeting->GetTarget());
			ChangeState(EnemyState::Attacking);
		}
		else
		{
			myMovement->SetMovementSpeed(myEnemyData.WalkSpeed);
			myAnimation->BlendTo(EnemyAnimationState::Walk);
			ChangeState(EnemyState::ReturnHome);
		}

		return;
	}
}

void RollingEnemyAIComponent::UpdateHurt(float aDeltaTime)
{
	if (myHurtTimer >= myHurtDuration)
	{
		Vector3f emissionDir = GetOwner()->GetTransform().GetPosition() - Essentials::GetPlayer()->GetTransform().GetPosition();
		myEmitterComponent->SetEmissionDirection(ParticleType::Blood, emissionDir);
		myEmitterComponent->Burst(ParticleType::Blood);
	}

	myHurtTimer -= aDeltaTime;

	if (myHurtTimer < 0.0f)
	{
		if (myFirstHit)
		{
			myAnimation->BlendTo(EnemyAnimationState::Aggro, 15.0f);
			myHurtTimer = myHurtDuration;
			myIsAggro = true;
			myFirstHit = false;
			ChangeState(EnemyState::React);
		}
		else
		{
			myAnimation->BlendTo(EnemyAnimationState::ChargeAttack, 15.0f);
			myHurtTimer = myHurtDuration;
			ChangeState(EnemyState::Attacking);
		}
	}
}

void RollingEnemyAIComponent::UpdateStunned(float aDeltaTime)
{
	GameObject* target = myTargeting->GetTarget();

	if (!target)
	{
		return;
	}

	myStunnedTimer -= aDeltaTime;

	if (myStunnedTimer < 0)
	{
		myAnimation->BlendTo(EnemyAnimationState::ChargeAttack);
		myAttack->StartAttack(target);
		ChangeState(EnemyState::Attacking);
		return;
	}
}

void RollingEnemyAIComponent::UpdateDeath(float aDeltaTime)
{
	StopRollingSound();

	if (myAttack)
	{
		myAttack->FinishRollingAttack();
	}

	myDeathTimer -= aDeltaTime;

	if (myDeathTimer < 0)
	{
		GetOwner()->SetActive(false);
	}
}

void RollingEnemyAIComponent::UpdateReturnHome(float aDeltaTime)
{
	if (myDamageableComponent->TookDamageThisFrame())
	{
		myAnimation->BlendTo(EnemyAnimationState::Hurt, 15.0f);
		ChangeState(EnemyState::Hurt);
		return;
	}

	if (myTargeting->IsTargetInRange())
	{
		myReactionTimer = 0.6f;
		myAnimation->BlendTo(EnemyAnimationState::Aggro, 15.0f);
		ChangeState(EnemyState::React);
		return;
	}

	MoveTowardsHome(aDeltaTime);
}