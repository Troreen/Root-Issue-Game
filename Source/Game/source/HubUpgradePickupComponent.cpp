#include "HubUpgradePickupComponent.h"

#include "AnimationGraphComponent.h"
#include "Essentials/Essentials.h"
#include "GameObject.h"
#include "Player/PlayerControllerComponent.h"
#include "SceneObjectData.h"
#include "WorldTransitionService.h"
#include "WorldTriggerHelpers.h"

#include <algorithm>
#include <iostream>
#include <utility>

namespace
{
	constexpr float kFusePlayerPickupDuration = 3.0f;
	constexpr float kAntennaPlayerPickupDuration = 1.2666667f;
	constexpr float kFuseItemPickupDuration = 3.3333333f;

	bool IsAntennaPickup(const std::string& aPickupKind)
	{
		return aPickupKind == "Antenna" || aPickupKind == "antenna";
	}
}

HubUpgradePickupComponent::HubUpgradePickupComponent(const SceneObjectData& aData)
{
	myPickupKind = aData.GetPropertyOr<std::string>("pickupKind", "Fuse");
	myTargetScene = aData.GetPropertyOr<std::string>("targetScene", "");
	myTargetSpawnId = aData.GetPropertyOr<std::string>("targetSpawnId", "");
	myFadeOutSeconds = (std::max)(0.0f, aData.GetPropertyOr<float>("fadeOutSeconds", 0.5f));
	myItemPickupParameter = aData.GetPropertyOr<std::string>("itemPickupParameter", "w_pickup");
	myItemAnimationDuration = aData.GetPropertyOr<float>("itemPickupDuration", kFuseItemPickupDuration);

	if (IsAntennaPickup(myPickupKind))
	{
		myPlayerAnimationParameter = aData.GetPropertyOr<std::string>("playerPickupParameter", "w_antenna_place");
		myPlayerAnimationDuration = aData.GetPropertyOr<float>("playerPickupDuration", kAntennaPlayerPickupDuration);
	}
	else
	{
		myPlayerAnimationParameter = aData.GetPropertyOr<std::string>("playerPickupParameter", "w_fuse_pickup");
		myPlayerAnimationDuration = aData.GetPropertyOr<float>("playerPickupDuration", kFusePlayerPickupDuration);
	}
}

void HubUpgradePickupComponent::OnStart()
{
	if (GameObject* owner = GetOwner())
	{
		myItemAnimationGraph = owner->GetComponent<AnimationGraphComponent>();
	}

	if (myItemAnimationGraph)
	{
		myItemAnimationGraph->SetFloatParameter(myItemPickupParameter, 0.0f);
	}
}

void HubUpgradePickupComponent::OnUpdate(const float aDeltaTime)
{
	if (mySequenceActive)
	{
		mySequenceTimer -= aDeltaTime;
		if (mySequenceTimer > 0.0f)
		{
			return;
		}

		mySequenceActive = false;
		if (!WorldTransitionService::RequestSceneTransition(myTargetScene, myTargetSpawnId, myFadeOutSeconds))
		{
			WorldTransitionService::EndSequence();
		}
		return;
	}

	GameObject* owner = GetOwner();
	if (!owner || myHasTriggered)
	{
		return;
	}

	const bool isInside = WorldTriggerHelpers::IsTriggerInside(*owner);
	const bool entered = isInside && !myWasInside;
	myWasInside = isInside;

	if (!entered)
	{
		return;
	}

	TryStartSequence();
}

bool HubUpgradePickupComponent::TryStartSequence()
{
	GameObject* owner = GetOwner();
	if (!owner)
	{
		return false;
	}

	if (myTargetScene.empty())
	{
		CancelSequence("has no targetScene");
		return false;
	}

	if (!myItemAnimationGraph)
	{
		myItemAnimationGraph = owner->GetComponent<AnimationGraphComponent>();
	}

	if (!myItemAnimationGraph)
	{
		CancelSequence("has no AnimationGraphComponent");
		return false;
	}

	GameObject* player = Essentials::GetPlayer();
	if (!player)
	{
		CancelSequence("could not find player");
		return false;
	}

	myPlayerController = player->GetComponent<PlayerControllerComponent>();
	if (!myPlayerController)
	{
		CancelSequence("player has no PlayerControllerComponent");
		return false;
	}

	if (!player->GetComponent<AnimationGraphComponent>())
	{
		CancelSequence("player has no AnimationGraphComponent");
		return false;
	}

	if (!WorldTransitionService::TryBeginSequence())
	{
		return false;
	}

	myItemAnimationGraph->SetFloatParameter(myItemPickupParameter, 1.0f);
	if (!myPlayerController->StartScriptedPickupAnimation(myPlayerAnimationParameter, myPlayerAnimationDuration))
	{
		myItemAnimationGraph->SetFloatParameter(myItemPickupParameter, 0.0f);
		CancelSequence("failed to start player pickup animation");
		WorldTransitionService::EndSequence();
		return false;
	}

	myHasTriggered = true;
	mySequenceActive = true;
	mySequenceTimer = (std::max)(myPlayerAnimationDuration, myItemAnimationDuration);
	return true;
}

void HubUpgradePickupComponent::CancelSequence(const std::string& aReason)
{
	if (GameObject* owner = GetOwner())
	{
		std::cout << "[HubUpgradePickup] '" << owner->GetName() << "' " << aReason << ".\n";
	}
	else
	{
		std::cout << "[HubUpgradePickup] Pickup " << aReason << ".\n";
	}
}
