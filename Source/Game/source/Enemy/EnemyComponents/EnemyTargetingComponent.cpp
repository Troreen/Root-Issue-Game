#include "EnemyTargetingComponent.h"
#include "EnemyAIComponent.h"
#include "Essentials.h"
#include "GameObject.h"

EnemyTargetingComponent::EnemyTargetingComponent()
{
}

EnemyTargetingComponent::~EnemyTargetingComponent()
{
}

void EnemyTargetingComponent::OnUpdate(float /*aDeltaTime*/)
{
	auto& playerPos = Essentials::GetPlayer()->GetTransform().GetPosition();
	auto& ownerPos = GetOwner()->GetTransform().GetPosition();

	Vector3f diff = playerPos - ownerPos;

	if (diff.Length() < myDetectionRange)
	{
		myTargetIsInRange = true;
	}
}

bool EnemyTargetingComponent::IsTargetInRange() const
{
	return myTargetIsInRange;
}

//void EnemyTargetingComponent::Update(float aDeltaTime)
//{
//	Essentials::GetPlayer();
//}
