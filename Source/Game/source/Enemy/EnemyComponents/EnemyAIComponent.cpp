#include "EnemyAIComponent.h"
#include "EnemyData.h"
#include "EnemyMovementComponent.h"
#include "EnemyTargetingComponent.h"
#include "GameObject.h"
#include "Essentials.h"
//EnemyAIComponent::EnemyAIComponent()
//{
//}

EnemyAIComponent::EnemyAIComponent(EnemyType aEnemyType)
{
	myType = aEnemyType;
	myPreviousState = BasicEnemyState::Idle;
	myCurrentState = myPreviousState;
}

EnemyAIComponent::~EnemyAIComponent()
{
}

void EnemyAIComponent::OnStart()
{
}

void EnemyAIComponent::OnUpdate(float aDeltaTime)
{
	AILogicUpdate(aDeltaTime);
}

void EnemyAIComponent::SetAggro(bool aState)
{
	myIsAggro = aState;
}

void EnemyAIComponent::BasicEnemyLogicUpdate(float aDeltaTime)
{
	auto* movement = GetOwner()->GetComponent<EnemyMovementComponent>();
	auto* targeting = GetOwner()->GetComponent<EnemyTargetingComponent>();

	if (!myIsAggro)
	{
		if (myCurrentState == BasicEnemyState::Idle)
		{
			movement->MoveRandomly(aDeltaTime);

			if (targeting->IsTargetInRange())
			{
				myIsAggro = true;
			}
		}
	}

	if (myIsAggro)
	{
		if (movement)
		{
			movement->MoveTowards(Essentials::GetPlayer(), aDeltaTime);
		}
	}

}

void EnemyAIComponent::RollingEnemyLogicUpdate(float /*aDeltaTime*/)
{
}

//void EnemyAIComponent::OnEnter()
//{
//}
//
//void EnemyAIComponent::OnExit()
//{
//}

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
