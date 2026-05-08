#pragma once

#include <string>

class WorldTransitionService
{
public:
	class Listener
	{
	public:
		virtual ~Listener() = default;
		virtual bool RequestSceneTransition(
			const std::string& aTargetScene,
			const std::string& aTargetSpawnId,
			float aFadeOutSeconds) = 0;
	};

	static void SetListener(Listener* aListener);
	static bool TryBeginSequence();
	static void EndSequence();
	static bool IsSequenceActive();
	static bool RequestSceneTransition(
		const std::string& aTargetScene,
		const std::string& aTargetSpawnId,
		float aFadeOutSeconds);

private:
	static Listener* ourListener;
	static bool ourSequenceActive;
};
