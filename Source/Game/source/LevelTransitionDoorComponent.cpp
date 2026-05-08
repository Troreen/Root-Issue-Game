#include "LevelTransitionDoorComponent.h"

#include "Essentials/Essentials.h"
#include "GameObject.h"
#include "Player/PlayerControllerComponent.h"
#include "WorldTransitionService.h"
#include "WorldTriggerHelpers.h"

#include <algorithm>
#include <iostream>
#include <utility>

LevelTransitionDoorComponent::LevelTransitionDoorComponent(
	std::string aTargetScene,
	std::string aTargetSpawnId,
	const float anAutoWalkSpeed,
	const float aFadeOutSeconds)
	: myTargetScene(std::move(aTargetScene))
	, myTargetSpawnId(std::move(aTargetSpawnId))
	, myAutoWalkSpeed((std::max)(1.0f, anAutoWalkSpeed))
	, myFadeOutSeconds((std::max)(0.0f, aFadeOutSeconds))
{
}

void LevelTransitionDoorComponent::OnUpdate(float /*aDeltaTime*/)
{
	GameObject* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	const bool isInside = WorldTriggerHelpers::IsTriggerInside(*owner);
	const bool entered = isInside && !myWasInside;
	if (!isInside)
	{
		myWasInside = false;
		return;
	}

	myWasInside = true;
	if (!entered || myHasTriggered)
	{
		return;
	}

	if (myTargetScene.empty())
	{
		std::cout << "[LevelTransitionDoor] Door '" << owner->GetName()
			<< "' has no targetScene.\n";
		return;
	}

	if (!WorldTransitionService::TryBeginSequence())
	{
		return;
	}

	GameObject* player = Essentials::GetPlayer();
	if (!player)
	{
		std::cout << "[LevelTransitionDoor] Could not find player for door '"
			<< owner->GetName() << "'.\n";
		WorldTransitionService::EndSequence();
		return;
	}

	auto* playerController = player->GetComponent<PlayerControllerComponent>();
	if (!playerController)
	{
		std::cout << "[LevelTransitionDoor] Player has no PlayerControllerComponent.\n";
		WorldTransitionService::EndSequence();
		return;
	}

	myHasTriggered = true;
	const WorldTriggerHelpers::Vector3f center = WorldTriggerHelpers::GetTriggerCenter(*owner);
	playerController->StartForcedMoveTo(
		center,
		myAutoWalkSpeed,
		[targetScene = myTargetScene, targetSpawnId = myTargetSpawnId, fadeOutSeconds = myFadeOutSeconds]()
		{
			if (!WorldTransitionService::RequestSceneTransition(targetScene, targetSpawnId, fadeOutSeconds))
			{
				WorldTransitionService::EndSequence();
			}
		});
}
