#include "MeleeEnemyAIComponent.h"

#include "EnemyMovementComponent.h"
#include "EnemyTargetingComponent.h"
#include "EnemyAnimationComponent.h"
#include "ParticleEmitterComponent.h"
#include "EnemyAttackComponent.h"
#include "DamageableComponent.h"
#include "GameObject.h"
#include "Essentials.h"

MeleeEnemyAIComponent::MeleeEnemyAIComponent(const EnemyData& someEnemyData) : EnemyAIComponent(someEnemyData)
{
}

void MeleeEnemyAIComponent::OnUpdate(float aDeltaTime)
{
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

	case EnemyState::Chasing:
		UpdateChasing(aDeltaTime);
		break;

	case EnemyState::Attacking:
	{
		if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eBasicAttackVox))
		{
			Essentials::globalAudioManager->PlaySFX(SoundID::eBasicAttackVox);
		}

		UpdateAttacking(aDeltaTime);
		break;
	}

	case EnemyState::Hurt:
	{
		if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eBasicVox))
		{
			Essentials::globalAudioManager->PlaySFX(SoundID::eBasicVox);
		}

		UpdateHurt(aDeltaTime);
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

void MeleeEnemyAIComponent::UpdateSpawn(float aDeltaTime)
{
	mySpawnTimer -= aDeltaTime;

	if (mySpawnTimer < 0.0f)
	{
		myAnimation->BlendTo(EnemyAnimationState::AggroWalk, 5.0f);

		myMovement->SetMovementSpeed(myEnemyData.ChaseSpeed);

		ChangeState(EnemyState::Chasing);
	}
}

void MeleeEnemyAIComponent::UpdateIdle(float aDeltaTime)
{
	myAnimation->BlendTo(EnemyAnimationState::Idle);

	myMovement->StopMoving();

	PickNewDirection();

	myIdleTimer -= aDeltaTime;

	if (myTargeting->IsTargetInRange())
	{
		myIsAggro = true;
		myAnimation->BlendTo(EnemyAnimationState::AggroWalk, 5.0f);
		myMovement->SetMovementSpeed(myEnemyData.ChaseSpeed);
		ChangeState(EnemyState::Chasing);

		return;
	}

	if (myDamageableComponent->TookDamageThisFrame())
	{
		myAnimation->BlendTo(EnemyAnimationState::Hurt, 15.0f);
		ChangeState(EnemyState::Hurt);
		return;
	}

	if (myIdleTimer <= 0.0f)
	{
		myAnimation->BlendTo(EnemyAnimationState::Walk);
		ChangeState(EnemyState::Wander);
		myWanderTimer = GetRandomFloat(1.5f, 3.0f);
	}
}

void MeleeEnemyAIComponent::UpdateWander(float aDeltaTime)
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
		myAnimation->BlendTo(EnemyAnimationState::AggroWalk, 5.0f);
		myMovement->SetMovementSpeed(myEnemyData.ChaseSpeed);
		ChangeState(EnemyState::Chasing);
		myIsAggro = true;
	}

	if (myWanderTimer <= 0.0f)
	{
		if (GetRandomFloat(0.0f, 1.0f) < 0.3f)
		{
			if (!myIsAggro)
			{
				myAnimation->BlendTo(EnemyAnimationState::Idle);
				ChangeState(EnemyState::Idle);
				myIdleTimer = GetRandomFloat(1.0f, 2.0f);
			}
			else
			{
				myAnimation->BlendTo(EnemyAnimationState::IdleAggro);
				ChangeState(EnemyState::Idle);
				myIdleTimer = GetRandomFloat(1.0f, 3.0f);
			}
		}
		else
		{
			myWanderTimer = GetRandomFloat(1.5f, 3.0f);
			PickNewDirection();
		}
	}
}

void MeleeEnemyAIComponent::UpdateChasing(float aDeltaTime)
{
	GameObject* target = myTargeting->GetTarget();

	if (!target)
	{
		return;
	}

	CommonUtilities::Vector3<float> targetPos = target->GetTransform().GetPosition();

	CommonUtilities::Vector3<float> currentPos = GetOwner()->GetTransform().GetPosition();

	float distanceToPlayer = (targetPos - currentPos).Length();

	float distanceFromHome = (currentPos - myStartPosition).Length();

	if (CheckIfDeadOrTookDamage())
	{
		return;
	}

	if (distanceToPlayer > myChaseRange && distanceFromHome > myLeashDistance)
	{
		ChangeState(EnemyState::ReturnHome);
		return;
	}

	if (distanceToPlayer <= myEnemyData.AttackRange)
	{
		if (myAttack->CanAttack())
		{
			myAnimation->BlendTo(EnemyAnimationState::Attack);
			myAttack->StartAttack(target);
			ChangeState(EnemyState::Attacking);
			return;
		}
	}

	myMovement->MoveTowardsTarget(target, aDeltaTime);
}

void MeleeEnemyAIComponent::UpdateAttacking(float)
{
	if (CheckIfDeadOrTookDamage())
	{
		return;
	}

	if (!myAttack->IsAttacking())
	{
		myAnimation->BlendTo(EnemyAnimationState::AggroWalk, 5.0f);
		ChangeState(EnemyState::Chasing);
	}
}

void MeleeEnemyAIComponent::UpdateHurt(float aDeltaTime)
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
		myAnimation->BlendTo(EnemyAnimationState::AggroWalk);
		myHurtTimer = myHurtDuration;
		myIsAggro = true;
		ChangeState(EnemyState::Chasing);
	}
}

void MeleeEnemyAIComponent::UpdateDeath(float aDeltaTime)
{
	myDeathTimer -= aDeltaTime;

	if (myDeathTimer < 0)
	{
		GetOwner()->SetActive(false);
	}
}

void MeleeEnemyAIComponent::UpdateReturnHome(float aDeltaTime)
{
	if (CheckIfDeadOrTookDamage())
	{
		return;
	}

	MoveTowardsHome(aDeltaTime);
}

bool MeleeEnemyAIComponent::CheckIfDeadOrTookDamage()
{
	if (myDamageableComponent->IsDead())
	{
		myAttack->CancelAttack();
		myMovement->StopMoving();
		myAnimation->BlendTo(EnemyAnimationState::Death, 10.0f);
		ChangeState(EnemyState::Death);
		return true;
	}

	if (myDamageableComponent->TookDamageThisFrame())
	{
		myAttack->CancelAttack();
		myMovement->StopMoving();
		myAnimation->BlendTo(EnemyAnimationState::Hurt, 15.0f);
		ChangeState(EnemyState::Hurt);
		return true;
	}

	return false;
}
