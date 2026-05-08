#pragma once

#include "ScriptComponent.h"

#include <string>

class LevelTransitionDoorComponent final : public ScriptComponent
{
public:
	LevelTransitionDoorComponent(
		std::string aTargetScene,
		std::string aTargetSpawnId,
		float anAutoWalkSpeed = 600.0f,
		float aFadeOutSeconds = 0.5f);

protected:
	void OnUpdate(float aDeltaTime) override;

private:
	std::string myTargetScene;
	std::string myTargetSpawnId;
	float myAutoWalkSpeed = 600.0f;
	float myFadeOutSeconds = 0.5f;
	bool myWasInside = false;
	bool myHasTriggered = false;
};
