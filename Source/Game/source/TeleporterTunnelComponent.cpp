#include "TeleporterTunnelComponent.h"

#include "Essentials/Essentials.h"
#include "GameObject.h"
#include "Player/PlayerControllerComponent.h"
#include "WorldTransitionService.h"
#include "WorldTriggerHelpers.h"
#include "SceneTransitionController.h"
#include "Essentials.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
	std::vector<TeleporterTunnelComponent*> gTeleporters;
}

TeleporterTunnelComponent::TeleporterTunnelComponent(
	const int aPairId,
	const int anExitDirection,
	const float anAutoWalkSpeed,
	const float anExitPadding)
	: myPairId(aPairId)
	, myExitDirection(NormalizeDirection(anExitDirection))
	, myAutoWalkSpeed((std::max)(1.0f, anAutoWalkSpeed))
	, myExitPadding((std::max)(0.0f, anExitPadding))
{
	if (anExitDirection < 0 || anExitDirection > 3)
	{
		std::cout << "[TeleporterTunnel] Invalid exitDirection " << anExitDirection
			<< " for pairId " << myPairId << ". Wrapped to " << myExitDirection << ".\n";
	}
}

int TeleporterTunnelComponent::GetPairId() const
{
	return myPairId;
}

int TeleporterTunnelComponent::GetExitDirection() const
{
	return myExitDirection;
}

void TeleporterTunnelComponent::SuppressUntilExit()
{
	mySuppressUntilExit = true;
	mySuppressHasSeenInside = false;
	myWasInside = true;
}

void TeleporterTunnelComponent::Receive(const Message& aMSG)
{
	aMSG;
}

void TeleporterTunnelComponent::OnStart()
{
	if (myHasRegistered)
	{
		return;
	}

	gTeleporters.push_back(this);
	myHasRegistered = true;
	ValidatePairCount(myPairId);
}

void TeleporterTunnelComponent::OnUpdate(float /*aDeltaTime*/)
{
	GameObject* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	const bool isInside = WorldTriggerHelpers::IsTriggerInside(*owner);
	if (mySuppressUntilExit)
	{
		if (isInside)
		{
			mySuppressHasSeenInside = true;
			myWasInside = true;
			return;
		}

		if (mySuppressHasSeenInside)
		{
			mySuppressUntilExit = false;
			mySuppressHasSeenInside = false;
			myWasInside = false;
		}
		return;
	}

	const bool entered = isInside && !myWasInside;
	if (!isInside)
	{
		myWasInside = false;
		return;
	}

	myWasInside = true;
	if (!entered || WorldTransitionService::IsSequenceActive())
	{
		return;
	}

	TeleporterTunnelComponent* destination = FindPairedTeleporter();
	if (!destination)
	{
		std::cout << "[TeleporterTunnel] Teleporter '" << owner->GetName()
			<< "' with pairId " << myPairId
			<< " needs exactly one paired teleporter.\n";
		return;
	}

	if (!WorldTransitionService::TryBeginSequence())
	{
		return;
	}

	StartTeleportSequence(*destination);
}

void TeleporterTunnelComponent::OnScriptDestroy()
{
	if (!myHasRegistered)
	{
		return;
	}

	gTeleporters.erase(
		std::remove(gTeleporters.begin(), gTeleporters.end(), this),
		gTeleporters.end());
	myHasRegistered = false;
}

TeleporterTunnelComponent* TeleporterTunnelComponent::FindPairedTeleporter() const
{
	TeleporterTunnelComponent* result = nullptr;
	int count = 0;
	for (TeleporterTunnelComponent* teleporter : gTeleporters)
	{
		if (!teleporter || teleporter->myPairId != myPairId)
		{
			continue;
		}

		++count;
		if (teleporter != this)
		{
			result = teleporter;
		}
	}

	if (count != 2)
	{
		return nullptr;
	}

	return result;
}

void TeleporterTunnelComponent::StartTeleportSequence(TeleporterTunnelComponent& aDestination)
{
	GameObject* sourceOwner = GetOwner();
	GameObject* destinationOwner = aDestination.GetOwner();
	GameObject* player = Essentials::GetPlayer();
	if (!sourceOwner || !destinationOwner || !player)
	{
		WorldTransitionService::EndSequence();
		return;
	}

	auto* playerController = player->GetComponent<PlayerControllerComponent>();
	if (!playerController)
	{
		std::cout << "[TeleporterTunnel] Player has no PlayerControllerComponent.\n";
		WorldTransitionService::EndSequence();
		return;
	}

	const WorldTriggerHelpers::Vector3f sourceCenter = WorldTriggerHelpers::GetTriggerCenter(*sourceOwner);
	TeleporterTunnelComponent* destination = &aDestination;
	playerController->StartForcedMoveTo(
		sourceCenter,
		myAutoWalkSpeed,
		[destination, sourceCenter]()
		{
			if (!destination)
			{
				WorldTransitionService::EndSequence();
				return;
			}

			GameObject* destinationOwner = destination->GetOwner();
			GameObject* player = Essentials::GetPlayer();
			if (!destinationOwner || !player)
			{
				WorldTransitionService::EndSequence();
				return;
			}

			auto* playerController = player->GetComponent<PlayerControllerComponent>();
			if (!playerController)
			{
				WorldTransitionService::EndSequence();
				return;
			}

			const WorldTriggerHelpers::Vector3f destinationCenter =
				WorldTriggerHelpers::GetTriggerCenter(*destinationOwner);
			WorldTriggerHelpers::Vector3f playerPosition = player->GetTransform().GetPosition();
			playerPosition.x = destinationCenter.x;
			playerPosition.y += (destinationCenter.y - sourceCenter.y);
			playerPosition.z = destinationCenter.z;
			player->GetTransform().SetPosition(playerPosition);

			const WorldTriggerHelpers::Vector3f direction =
				DirectionFromIndex(destination->GetExitDirection());
			playerController->FaceDirection(direction);
			destination->SuppressUntilExit();

			const WorldTriggerHelpers::Vector3f halfExtents =
				WorldTriggerHelpers::GetTriggerHalfExtents(*destinationOwner);
			const float axisHalfExtent = (std::abs(direction.x) > 0.0f) ? halfExtents.x : halfExtents.z;
			WorldTriggerHelpers::Vector3f exitTarget = destinationCenter + direction * (axisHalfExtent + destination->myExitPadding);
			exitTarget.y = playerPosition.y;

			PostMaster* PostMaster = Essentials::globalPostMaster.get();

			Message a;
			a.myMessageType = MessageType::LoadScene;
			PostMaster->SendMsg(a);

			playerController->StartForcedMoveTo(
				exitTarget,
				destination->myAutoWalkSpeed,
				[]()
				{
					WorldTransitionService::EndSequence();
				});

			std::cout << "[TeleporterTunnel] Teleported player to (" << destinationCenter.x << ", " << destinationCenter.y << ", " << destinationCenter.z
				<< ") and started forced move to (" << exitTarget.x << ", " << exitTarget.y << ", " << exitTarget.z
				<< ") with speed " << destination->myAutoWalkSpeed << ".\n";
		});
}

CommonUtilities::Vector3<float> TeleporterTunnelComponent::DirectionFromIndex(const int aDirection)
{
	switch (NormalizeDirection(aDirection))
	{
	case 1:
		return { 1.0f, 0.0f, 0.0f };
	case 2:
		return { 0.0f, 0.0f, -1.0f };
	case 3:
		return { -1.0f, 0.0f, 0.0f };
	case 0:
	default:
		return { 0.0f, 0.0f, 1.0f };
	}
}

int TeleporterTunnelComponent::NormalizeDirection(const int aDirection)
{
	const int wrapped = aDirection % 4;
	return wrapped < 0 ? wrapped + 4 : wrapped;
}

void TeleporterTunnelComponent::ValidatePairCount(const int aPairId)
{
	int count = 0;
	for (const TeleporterTunnelComponent* teleporter : gTeleporters)
	{
		if (teleporter && teleporter->myPairId == aPairId)
		{
			++count;
		}
	}

	if (count > 2)
	{
		std::cout << "[TeleporterTunnel] More than two teleporters use pairId "
			<< aPairId << ". This content is invalid.\n";
		assert(false && "More than two teleporters use the same pairId.");
	}
}
