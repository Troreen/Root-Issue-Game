#pragma once

#include "ScriptComponent.h"

#include <CommonUtilities/Vector3.hpp>

class TeleporterTunnelComponent final : public ScriptComponent
{
public:
	TeleporterTunnelComponent(
		int aPairId,
		int anExitDirection,
		float anAutoWalkSpeed = 600.0f,
		float anExitPadding = 90.0f);

	int GetPairId() const;
	int GetExitDirection() const;
	void SuppressUntilExit();

protected:
	void OnStart() override;
	void OnUpdate(float aDeltaTime) override;
	void OnScriptDestroy() override;

private:
	TeleporterTunnelComponent* FindPairedTeleporter() const;
	void StartTeleportSequence(TeleporterTunnelComponent& aDestination);
	static CommonUtilities::Vector3<float> DirectionFromIndex(int aDirection);
	static int NormalizeDirection(int aDirection);
	static void ValidatePairCount(int aPairId);

	int myPairId = 0;
	int myExitDirection = 0;
	float myAutoWalkSpeed = 600.0f;
	float myExitPadding = 90.0f;
	bool myWasInside = false;
	bool mySuppressUntilExit = false;
	bool mySuppressHasSeenInside = false;
	bool myHasRegistered = false;
};
