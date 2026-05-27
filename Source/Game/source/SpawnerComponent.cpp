#include "SpawnerComponent.h"
#include "GameObject.h"
#include "Essentials/Essentials.h"
#include "EnemyAIComponent.h"

void SpanwnerComponent::OnStart()
{
	Essentials::PushEnemyInto(myEnemies);
	myIsTrigger = false;
	myTriggerRadius = 100.f;
	myIndex = 0;
}

void SpanwnerComponent::OnUpdate(float)
{
	if (myIsTrigger)
	{
		return;
	}

	GameObject* player = Essentials::GetPlayer();

	if (!player)
	{
		return;
	}

	Vector3f playerPosition = player->GetTransform().GetPosition();
	Vector3f ownerPosition = GetOwner()->GetTransform().GetPosition();
	Vector3f distanceVector = playerPosition - ownerPosition;

	float distance = distanceVector.LengthSqr();
	if (myTriggerRadius * myTriggerRadius > distance)
	{
		myIsTrigger = true;

		for (auto& enemy : myEnemies)
		{
			enemy->Spawn();
		}
	}
}

void SpanwnerComponent::Reset()
{
	myIsTrigger = false;
}

void SpanwnerComponent::SetRadius(float aRadius)
{
	myTriggerRadius = aRadius;
}
