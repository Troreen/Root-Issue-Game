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
	myCurrentState = BasicEnemyState::Idle;
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
	GetOwner()->SetActive(true);
	
	ResetAnimations();

	myCurrentState = BasicEnemyState::Idle;
	myIsAggro = false;
	myAnimationWeight = 0.0f;
	myIdleTimer = GetRandomFloat(1.0f, 2.0f);
	myWanderTimer = GetRandomFloat(1.5f, 3.0f);
	myDeathTimer = 3.0f;
	myMaxSpeed = 450.0f;
}

void EnemyAIComponent::HandleStatesBasicEnemy(float aDeltaTime)
{
	switch (myCurrentState)
	{
	case BasicEnemyState::Idle:
		UpdateIdle(aDeltaTime);
		break;
	case BasicEnemyState::Wander:
		UpdateWander(aDeltaTime);
		break;
	case BasicEnemyState::Chasing:
		UpdateChasing(aDeltaTime);
		break;
	case BasicEnemyState::Attacking:
		UpdateAttacking(aDeltaTime);
		break;
	case BasicEnemyState::Hurt:
		UpdateHurt(aDeltaTime);
		break;
	case BasicEnemyState::Death:
		UpdateDeath(aDeltaTime);
		break;
	default:
		break;
	}
}

void EnemyAIComponent::ChangeState(const BasicEnemyState& aState)
{
	if (aState != myCurrentState)
	{
		myCurrentState = aState;
		std::cout << "Swapped from state: " << StringifyState(myPreviousState) << std::endl;
		std::cout << "To state: " << StringifyState(myCurrentState) << std::endl;
		myPreviousState = myCurrentState;
	}
}

std::string EnemyAIComponent::StringifyState(const BasicEnemyState& aState) const
{
	switch (aState)
	{
	case BasicEnemyState::Idle:
		return "Idle";
	case BasicEnemyState::Wander:
		return "Wander";
	case BasicEnemyState::Chasing:
		return "Chasing";
	case BasicEnemyState::Attacking:
		return "Attacking";
	case BasicEnemyState::Hurt:
		return "Hurt";
	case BasicEnemyState::Death:
		return "Death";
	default:
		return "Unknown";
	}
}

void EnemyAIComponent::UpdateIdle(float aDeltaTime)
{
	myAnimationGraph->SetFloatParameter("w_idle", 0);

	myMovement->StopMoving();

	PickNewDirection();

	myIdleTimer -= aDeltaTime;

	if (myIdleTimer <= 0.0f)
	{
		myAnimationGraph->SetFloatParameter("w_walk", 1.0f);
		ChangeState(BasicEnemyState::Wander);
		myWanderTimer = GetRandomFloat(1.5f, 3.0f);
	}

}

void EnemyAIComponent::UpdateWander(float aDeltaTime)
{
	myMovement->RotateTowards(myWanderDirection, aDeltaTime);
	myMovement->MoveForward(aDeltaTime);

	myWanderTimer -= aDeltaTime;

	if (myTargeting->IsTargetInRange())
	{
		myAnimationGraph->SetFloatParameter("w_walk", 0);
		myAnimationGraph->SetFloatParameter("w_aggro_walk", 1.0f);
		ChangeState(BasicEnemyState::Chasing);
		myIsAggro = true;
	}

	if (myWanderTimer <= 0.0f)
	{
		if (GetRandomFloat(0.0f, 1.0f) < 0.3f)
		{
			myAnimationGraph->SetFloatParameter("w_walk", 0);
			ChangeState(BasicEnemyState::Idle);
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
		ChangeState(BasicEnemyState::Death);
		return;
	}

	if (myDamageableComponent->TookDamageThisFrame())
	{
		myAnimationGraph->SetFloatParameter("w_aggro_walk", 0.f);
		myAnimationGraph->SetFloatParameter("w_hurt", 1.0f);
		ChangeState(BasicEnemyState::Hurt);
		return;
	}

	if (distance <= myEnemyData.AttackRange)
	{
		if (myAttack->CanAttack())
		{
			myAnimationGraph->SetFloatParameter("w_aggro_walk", 0);
			myAnimationGraph->SetFloatParameter("w_attack", 1.0f);
			myAttack->StartAttack(target);
			ChangeState(BasicEnemyState::Attacking);
			return;
		}
	}

	myMovement->MoveTowardsTarget(target, aDeltaTime);
}

void EnemyAIComponent::UpdateAttacking(float aDeltaTime)
{
	aDeltaTime;

	if (myDamageableComponent->IsDead())
	{
		myAnimationGraph->SetFloatParameter("w_death", 1.0f);
		ChangeState(BasicEnemyState::Death);
		return;
	}

	if (myDamageableComponent->TookDamageThisFrame())
	{
		myAnimationGraph->SetFloatParameter("w_hurt", 1.0f);
		myAnimationGraph->SetFloatParameter("w_attack", 0.0f);
		ChangeState(BasicEnemyState::Hurt);
		return;
	}

	if (!myAttack->IsAttacking())
	{
		myAnimationGraph->SetFloatParameter("w_attack", 0.0f);
		myAnimationGraph->SetFloatParameter("w_aggro_walk", 1.0f);
		ChangeState(BasicEnemyState::Chasing);
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
			ChangeState(BasicEnemyState::Chasing);
		}
		else
		{
			myIsAggro = true;
			myAnimationGraph->SetFloatParameter("w_aggro_walk", 1.0f);
			myAnimationGraph->SetFloatParameter("w_hurt", 0.0f);
			myHurtTimer = 0.5f;
			ChangeState(BasicEnemyState::Chasing);
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

void EnemyAIComponent::HandleStatesRollingEnemy()
{
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

void EnemyAIComponent::RollingEnemyLogicUpdate(float /*aDeltaTime*/)
{
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
