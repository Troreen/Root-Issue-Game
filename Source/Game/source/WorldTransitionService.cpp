#include "WorldTransitionService.h"

#include <iostream>

WorldTransitionService::Listener* WorldTransitionService::ourListener = nullptr;
bool WorldTransitionService::ourSequenceActive = false;

void WorldTransitionService::SetListener(Listener* aListener)
{
	ourListener = aListener;
}

bool WorldTransitionService::TryBeginSequence()
{
	if (ourSequenceActive)
	{
		return false;
	}

	ourSequenceActive = true;
	return true;
}

void WorldTransitionService::EndSequence()
{
	ourSequenceActive = false;
}

bool WorldTransitionService::IsSequenceActive()
{
	return ourSequenceActive;
}

bool WorldTransitionService::RequestSceneTransition(
	const std::string& aTargetScene,
	const std::string& aTargetSpawnId,
	const float aFadeOutSeconds)
{
	if (ourListener)
	{
		return ourListener->RequestSceneTransition(aTargetScene, aTargetSpawnId, aFadeOutSeconds);
	}

	std::cout << "[WorldTransition] No listener registered for scene transition to '"
		<< aTargetScene << "'.\n";
	EndSequence();
	return false;
}
