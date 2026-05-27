#pragma once

#include "ScriptComponent.h"

#include <string>

struct SceneObjectData;

class AnimationGraphComponent;
class PlayerControllerComponent;

class HubUpgradePickupComponent final : public ScriptComponent
{
public:
	explicit HubUpgradePickupComponent(const SceneObjectData& aData);

protected:
	void OnStart() override;
	void OnUpdate(float aDeltaTime) override;

private:
	bool TryStartSequence();
	void CancelSequence(const std::string& aReason);

	std::string myPickupKind;
	std::string myTargetScene;
	std::string myTargetSpawnId;
	std::string myPlayerAnimationParameter;
	std::string myItemPickupParameter = "w_pickup";
	float myFadeOutSeconds = 0.5f;
	float myPlayerAnimationDuration = 0.0f;
	float myItemAnimationDuration = 0.0f;
	float mySequenceTimer = 0.0f;
	bool myWasInside = false;
	bool myHasTriggered = false;
	bool mySequenceActive = false;
	AnimationGraphComponent* myItemAnimationGraph = nullptr;
	PlayerControllerComponent* myPlayerController = nullptr;
};
