#include "EnemyAIComponent.h"
#include "EnemyMovementComponent.h"
#include "EnemyTargetingComponent.h"
#include "AnimatedMeshComponent.h"
#include "AnimationGraphComponent.h"
#include "ParticleEmitterComponent.h"
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

EnemyAIComponent::EnemyAIComponent(EnemyType aEnemyType)
{
	myType = aEnemyType;
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


	//Debug
	if (!myMovement)
	{
		std::cout << "Error: Movement is nullptr" << std::endl;
	}
	if (!myTargeting)
	{
		std::cout << "Error: Targeting is nullptr" << std::endl;
	}
	if (!myAnimation)
	{
		std::cout << "Error: AnimationMesh is nullptr" << std::endl;
	}
	if (!myAnimationGraph)
	{
		std::cout << "Error: AnimationGraph is nullptr" << std::endl;
	}
	if (!myEmitterComponent)
	{
		std::cout << "Error: EmitterComponent is nullptr" << std::endl;
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
	//myMovement = GetOwner()->GetComponent<EnemyMovementComponent>();
	//myTargeting = GetOwner()->GetComponent<EnemyTargetingComponent>();
	//myAnimation = GetOwner()->GetComponent<AnimatedMeshComponent>();
	//myAnimationGraph = GetOwner()->GetComponent<AnimationGraphComponent>();

	////Debug
	//if (!myMovement)
	//{
	//	std::cout << "Error: Movement is nullptr" << std::endl;
	//}
	//if (!myTargeting)
	//{
	//	std::cout << "Error: Targeting is nullptr" << std::endl;
	//}
	//if (!myAnimation)
	//{
	//	std::cout << "Error: AnimationMesh is nullptr" << std::endl;
	//}
	//if (!myAnimationGraph)
	//{
	//	std::cout << "Error: AnimationGraph is nullptr" << std::endl;
	//}

	//myIdleTimer = Random(1.0f, 2.0f);
	//myWanderTimer = Random(1.5f, 3.0f);

	//PickNewDirection();

	myAnimationWeight = 1.0f;
	myAnimationGraph->SetFloatParameter("w_walk", myAnimationWeight);
}

void EnemyAIComponent::OnUpdate(float aDeltaTime)
{
	if (!myMovement || !myTargeting || !myAnimation || !myAnimationGraph)
	{
		return;
	}

	AILogicUpdate(aDeltaTime);
}

void EnemyAIComponent::SetAggro(bool aState)
{
	myIsAggro = aState;
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
	case BasicEnemyState::Death:
		return "Death";
	default:
		return "Unknown";
	}
}

void EnemyAIComponent::UpdateIdle(float aDeltaTime)
{
	myMovement->StopMoving();

	PickNewDirection();

	myIdleTimer -= aDeltaTime;

	if (myIdleTimer <= 0.0f)
	{
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
		ChangeState(BasicEnemyState::Chasing);
		myIsAggro = true;
	}

	if (myWanderTimer <= 0.0f)
	{
		if (GetRandomFloat(0.0f, 1.0f) < 0.3f)
		{
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
	
	myMovement->MoveTowardsTarget(target, aDeltaTime);

}

void EnemyAIComponent::UpdateAttacking(float aDeltaTime)
{
	aDeltaTime;
}

void EnemyAIComponent::UpdateDeath(float aDeltaTime)
{
	aDeltaTime;
}

void EnemyAIComponent::PickNewDirection()
{
	float angleZ = GetRandomAngleDegreeToRad(-30.0f, 30.0f);
	float angleX = GetRandomAngleDegreeToRad(-30.0f, 30.0f);

	myWanderDirection.x = angleX;
	myWanderDirection.z = angleZ;
	myWanderDirection.y = 0.0f;
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

	myAnimationWeight = 1.0f;

	myAnimationGraph->SetFloatParameter("w_walk", myAnimationWeight);
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
