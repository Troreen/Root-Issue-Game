#include "EnemyAIComponent.h"
#include "EnemyMovementComponent.h"
#include "EnemyTargetingComponent.h"
#include "AnimatedMeshComponent.h"
#include "AnimationGraphComponent.h"
#include "ParticleEmitterComponent.h"
#include "EnemyAttackComponent.h"
#include "DamageableComponent.h"
#include "GameObject.h"
#include "Essentials.h"
//EnemyAIComponent::EnemyAIComponent()
//{
//}

float EnemyAIComponent::GetRandomAngleDegreeToRad(float aMin, float aMax)
{
	float pi = 3.14159f;
	float minRadians = aMin * pi / 180.0f;
	float maxRadians = aMax * pi / 180.0f;

	std::uniform_real_distribution<float> rndDist(minRadians, maxRadians);

	return rndDist(myRandomEngine);
}

float EnemyAIComponent::GetRandomFloat(float aMin, float aMax)
{
	std::uniform_real_distribution<float> rndDist(aMin, aMax);

	return rndDist(myRandomEngine);
}

EnemyAIComponent::EnemyAIComponent(const EnemyData& someEnemyData)
{
	myEnemyData = someEnemyData;
	myType = someEnemyData.EnemyType;

	if (myEnemyData.ShouldSpawn)
	{
		myCurrentState = EnemyState::Spawn;
	}
	else
	{
		myCurrentState = EnemyState::Idle;
	}

	myPreviousState = myCurrentState;
	myAnimationWeight = 0.0f;
}

EnemyAIComponent::~EnemyAIComponent()
{
}

void EnemyAIComponent::Init(Tga::Engine& /*aEngine*/)
{
	if (myHasBeenInitialized)
	{
		return;
	}

	myMovement = GetOwner()->GetComponent<EnemyMovementComponent>();
	myTargeting = GetOwner()->GetComponent<EnemyTargetingComponent>();
	myAnimation = GetOwner()->GetComponent<AnimatedMeshComponent>();
	myAnimationGraph = GetOwner()->GetComponent<AnimationGraphComponent>();
	myEmitterComponent = GetOwner()->GetComponent<ParticleEmitterComponent>();
	myAttack = GetOwner()->GetComponent<EnemyAttackComponent>();
	myDamageableComponent = GetOwner()->GetComponent<DamageableComponent>();


	//Debug
	if (!myMovement)
	{
		std::cout << "Error: Movement is nullptr in AI" << std::endl;
	}
	if (!myTargeting)
	{
		std::cout << "Error: Targeting is nullptr in AI" << std::endl;
	}
	if (!myAnimation)
	{
		std::cout << "Error: AnimationMesh is nullptr in AI" << std::endl;
	}
	if (!myAnimationGraph)
	{
		std::cout << "Error: AnimationGraph is nullptr in AI" << std::endl;
	}
	if (!myEmitterComponent)
	{
		std::cout << "Error: EmitterComponent is nullptr in AI" << std::endl;
	}
	if (!myAttack)
	{
		std::cout << "Error: myAttack is nullptr in AI" << std::endl;
	}
	if (!myDamageableComponent)
	{
		std::cout << "Error: myDamageableComponent is nullptr in AI" << std::endl;
	}

	//// TODO: testing of particles
	/*myEmitterComponent->SetContinuousEmission(ParticleType::Blood, true);
	myEmitterComponent->SetContinuousEmission(ParticleType::Test, true);*/
	////end

	std::random_device seed;
	std::mt19937 rndEngine(seed());

	myRandomEngine = rndEngine;

	myIdleTimer = GetRandomFloat(1.0f, 2.0f);
	myWanderTimer = GetRandomFloat(1.5f, 3.0f);
	mySpawnTimer = myEnemyData.SpawnTime;

	PickNewDirection();

	myHasBeenInitialized = true;
}

void EnemyAIComponent::OnStart()
{
	myAnimationWeight = 0.0f;

}

void EnemyAIComponent::OnUpdate(float aDeltaTime)
{
	if (!myMovement || !myTargeting || !myAnimation || !myAnimationGraph || !myAttack)
	{
		return;
	}

	AILogicUpdate(aDeltaTime);
}

void EnemyAIComponent::SetAggro(bool aState)
{
	myIsAggro = aState;
}

void EnemyAIComponent::Reset()
{
	if (!myActiveAfterSave) return;

	GetOwner()->SetActive(true);
	
	ResetAnimations();

	myCurrentState = EnemyState::Idle;
	myIsAggro = false;
	myAnimationWeight = 0.0f;
	myIdleTimer = GetRandomFloat(1.0f, 2.0f);
	myWanderTimer = GetRandomFloat(1.5f, 3.0f);
	myDeathTimer = 3.0f;
	myMaxSpeed = 450.0f;
}

void EnemyAIComponent::Save()
{
	myActiveAfterSave = myCurrentState != EnemyState::Death;
}

void EnemyAIComponent::HandleStatesBasicEnemy(float aDeltaTime)
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
		if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eBasicAttackVox))
		{
			Essentials::globalAudioManager->PlaySFX(SoundID::eBasicAttackVox);
		}
		UpdateAttacking(aDeltaTime);
		break;
	case EnemyState::Hurt:
		if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eBasicVox))
		{
			Essentials::globalAudioManager->PlaySFX(SoundID::eBasicVox);
		}
		UpdateHurt(aDeltaTime);
		break;
	case EnemyState::Death:
		if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eEnemyDeadVox))
		{
			Essentials::globalAudioManager->PlaySFX(SoundID::eEnemyDeadVox);
		}
		UpdateDeath(aDeltaTime);
		break;
	default:
		break;
	}
}

void EnemyAIComponent::ChangeState(const EnemyState& aState)
{
	if (aState != myCurrentState)
	{
		myCurrentState = aState;
		std::cout << "Swapped from state: " << StringifyState(myPreviousState) << std::endl;
		std::cout << "To state: " << StringifyState(myCurrentState) << std::endl;
		myPreviousState = myCurrentState;
	}
}

std::string EnemyAIComponent::StringifyState(const EnemyState& aState) const
{
	switch (aState)
	{
	case EnemyState::Idle:
		return "Idle";
	case EnemyState::Wander:
		return "Wander";
	case EnemyState::Chasing:
		return "Chasing";
	case EnemyState::Attacking:
		return "Attacking";
	case EnemyState::Hurt:
		return "Hurt";
	case EnemyState::Death:
		return "Death";
	default:
		return "Unknown";
	}
}

void EnemyAIComponent::UpdateSpawn(float aDeltaTime)
{
	mySpawnTimer -= aDeltaTime;

	if (mySpawnTimer < 0.0f)
	{
		myAnimationGraph->SetFloatParameter("w_spawn", 0.0f);
		myAnimationGraph->SetFloatParameter("w_aggro_walk", 1.0f);
		myMovement->SetMovementSpeed(myEnemyData.ChaseSpeed);
		ChangeState(EnemyState::Chasing);
	}
}

void EnemyAIComponent::UpdateIdle(float aDeltaTime)
{
	myAnimationGraph->SetFloatParameter("w_idle", 0);

	myMovement->StopMoving();

	PickNewDirection();

	myIdleTimer -= aDeltaTime;

	if (myDamageableComponent->TookDamageThisFrame())
	{
		myAnimationGraph->SetFloatParameter("w_hurt", 1.0f);
		ChangeState(EnemyState::Hurt);
		return;
	}

	if (myIdleTimer <= 0.0f)
	{
		myAnimationGraph->SetFloatParameter("w_walk", 1.0f);
		ChangeState(EnemyState::Wander);
		myWanderTimer = GetRandomFloat(1.5f, 3.0f);
	}

}

void EnemyAIComponent::UpdateWander(float aDeltaTime)
{
	myMovement->RotateTowards(myWanderDirection, aDeltaTime);
	myMovement->MoveForward(aDeltaTime);

	myWanderTimer -= aDeltaTime;

	if (myDamageableComponent->TookDamageThisFrame())
	{
		myAnimationGraph->SetFloatParameter("w_walk", 0.f);
		myAnimationGraph->SetFloatParameter("w_hurt", 1.0f);
		ChangeState(EnemyState::Hurt);
		return;
	}

	if (myTargeting->IsTargetInRange())
	{
		myAnimationGraph->SetFloatParameter("w_walk", 0);
		myAnimationGraph->SetFloatParameter("w_aggro_walk", 1.0f);
		myMovement->SetMovementSpeed(myEnemyData.ChaseSpeed);
		ChangeState(EnemyState::Chasing);
		myIsAggro = true;
	}

	if (myWanderTimer <= 0.0f)
	{
		if (GetRandomFloat(0.0f, 1.0f) < 0.3f)
		{
			myAnimationGraph->SetFloatParameter("w_walk", 0);
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

void EnemyAIComponent::UpdateChasing(float aDeltaTime)
{
	GameObject* target = Essentials::GetPlayer();

	if (!target)
	{
		return;
	}

	float distance = (target->GetTransform().GetPosition() -
		GetOwner()->GetTransform().GetPosition()).Length();

	if (myDamageableComponent->IsDead())
	{
		myAnimationGraph->SetFloatParameter("w_death", 1.0f);
		myAnimationGraph->SetFloatParameter("w_aggro_walk", 0.f);
		ChangeState(EnemyState::Death);
		return;
	}

	if (myDamageableComponent->TookDamageThisFrame())
	{
		myAnimationGraph->SetFloatParameter("w_aggro_walk", 0.f);
		myAnimationGraph->SetFloatParameter("w_hurt", 1.0f);
		ChangeState(EnemyState::Hurt);
		return;
	}

	if (distance <= myEnemyData.AttackRange)
	{
		if (myAttack->CanAttack())
		{
			if (myType == EnemyType::BasicEnemy)
			{
				myAnimationGraph->SetFloatParameter("w_aggro_walk", 0);
				myAnimationGraph->SetFloatParameter("w_attack", 1.0f);
			}
			else
			{
				myAnimationGraph->SetFloatParameter("w_charge", 1.0f);
				myAnimationGraph->SetFloatParameter("w_walk", 0.0f);

			}
			myAttack->StartAttack(target);
			ChangeState(EnemyState::Attacking);
			return;
		}
	}

	myMovement->MoveTowardsTarget(target, aDeltaTime);
}

void EnemyAIComponent::UpdateAttacking(float aDeltaTime)
{
	aDeltaTime;
	if (myType == EnemyType::BasicEnemy)
	{
		if (myDamageableComponent->IsDead())
		{
			myAnimationGraph->SetFloatParameter("w_death", 1.0f);
			ChangeState(EnemyState::Death);
			return;
		}

		if (myDamageableComponent->TookDamageThisFrame())
		{
			myAnimationGraph->SetFloatParameter("w_hurt", 1.0f);
			myAnimationGraph->SetFloatParameter("w_attack", 0.0f);
			ChangeState(EnemyState::Hurt);
			return;
		}

		if (!myAttack->IsAttacking())
		{
			myAnimationGraph->SetFloatParameter("w_attack", 0.0f);
			myAnimationGraph->SetFloatParameter("w_aggro_walk", 1.0f);
			ChangeState(EnemyState::Chasing);
		}
	}
	else
	{
		if (myDamageableComponent->IsDead())
		{
			myAnimationGraph->SetFloatParameter("w_death_standing", 1.0f);
			ChangeState(EnemyState::Death);
			return;
		}

		if (myDamageableComponent->TookDamageThisFrame())
		{
			myAnimationGraph->SetFloatParameter("w_hurt", 1.0f);
			myAnimationGraph->SetFloatParameter("w_charge", 0.0f);
			ChangeState(EnemyState::Hurt);
			return;
		}

		if (!myAttack->IsAttacking())
		{
			myAnimationGraph->SetFloatParameter("w_walk", 1.0f);
			myAnimationGraph->SetFloatParameter("w_knockback", 0.0f);
			ChangeState(EnemyState::Chasing);
		}
	}
}

void EnemyAIComponent::UpdateHurt(float aDeltaTime)
{
	// Play hurt animation and spawn particle
	// If hurt animation is over, state can change to different state

	myHurtTimer -= aDeltaTime;

	Vector3f emissionDir = GetOwner()->GetTransform().GetPosition() - Essentials::GetPlayer()->GetTransform().GetPosition();
	myEmitterComponent->SetEmissionDirection(ParticleType::Blood, emissionDir);
	myEmitterComponent->Burst(ParticleType::Blood);

	if (myHurtTimer < 0.0f)
	{
		if (myIsAggro)
		{
			myAnimationGraph->SetFloatParameter("w_aggro_walk", 1.0f);
			myAnimationGraph->SetFloatParameter("w_hurt", 0.0f);
			myHurtTimer = 0.5f;
			ChangeState(EnemyState::Chasing);
		}
		else
		{
			myIsAggro = true;
			myAnimationGraph->SetFloatParameter("w_aggro_walk", 1.0f);
			myAnimationGraph->SetFloatParameter("w_hurt", 0.0f);
			myHurtTimer = 0.5f;
			ChangeState(EnemyState::Chasing);
		}

	}
}

void EnemyAIComponent::UpdateDeath(float aDeltaTime)
{
	myDeathTimer -= aDeltaTime;

	if (myDeathTimer < 0)
	{
		GetOwner()->SetActive(false);
		ResetAnimations();
	}
}

void EnemyAIComponent::PickNewDirection()
{
	float angleZ = GetRandomAngleDegreeToRad(-30.0f, 30.0f);
	float angleX = GetRandomAngleDegreeToRad(-30.0f, 30.0f);

	myWanderDirection.x = angleX;
	myWanderDirection.z = angleZ;
	myWanderDirection.y = 0.0f;
}

void EnemyAIComponent::ResetAnimations()
{
	myAnimationGraph->SetFloatParameter("w_walk", 0.0f);
	myAnimationGraph->SetFloatParameter("w_death", 0.0f);
	myAnimationGraph->SetFloatParameter("w_aggro_walk", 0.0f);
	myAnimationGraph->SetFloatParameter("w_idle", 0.0f);
	myAnimationGraph->SetFloatParameter("w_attack", 0.0f);
	myAnimationGraph->SetFloatParameter("w_hurt", 0.0f);
}

void EnemyAIComponent::HandleStatesRollingEnemy(float aDeltaTime)
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
		if (Essentials::globalAudioManager->IsEventPlaying(SoundID::eRoll))
		{
			Essentials::globalAudioManager->StopMusic(SoundID::eRoll, true);
		}
		UpdateChasing(aDeltaTime);
		break;
	case EnemyState::Attacking:
		if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eRoll))
		{
			Essentials::globalAudioManager->PlaySFX(SoundID::eRoll);
		}
		UpdateAttacking(aDeltaTime);
		break;
	case EnemyState::Hurt:
		if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eHeavyVox))
		{
			Essentials::globalAudioManager->PlaySFX(SoundID::eHeavyVox);
		}
		UpdateHurt(aDeltaTime);
		break;
	case EnemyState::Death:
		if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eEnemyDeadVox))
		{
			Essentials::globalAudioManager->PlaySFX(SoundID::eEnemyDeadVox);
		}
		UpdateDeath(aDeltaTime);
		break;
	default:
		break;
	}
}

void EnemyAIComponent::BasicEnemyLogicUpdate(float aDeltaTime)
{
	HandleStatesBasicEnemy(aDeltaTime);

	/*myAnimationWeight = 1.0f;
	myAnimationGraph->SetFloatParameter("w_walk", myAnimationWeight);*/

	/*float speed = myMovement->GetVelocity().Length();
	float normalized = speed / myMaxSpeed;

	normalized = std::clamp(normalized, 0.0f, 1.0f);*/
}

void EnemyAIComponent::RollingEnemyLogicUpdate(float aDeltaTime)
{
	HandleStatesRollingEnemy(aDeltaTime);
}

void EnemyAIComponent::AILogicUpdate(float aDeltaTime)
{
	switch (myType)
	{
	case EnemyType::BasicEnemy:
		BasicEnemyLogicUpdate(aDeltaTime);
		break;
	case EnemyType::RollingEnemy:
		RollingEnemyLogicUpdate(aDeltaTime);
		break;
	default:
		break;
	}
}
