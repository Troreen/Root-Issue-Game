#pragma once
#include "ScriptComponent.h"
#include "PlayerState.h"
#include "CommonUtilities/Vector.hpp"
#include "GameObject.h"
#include <functional>
#include <vector>

class PlayerControllerComponent : public ScriptComponent
{
public:
	using ForcedMoveCompleteCallback = std::function<void()>;

	PlayerControllerComponent() = default;
	~PlayerControllerComponent() = default;

	void Reset() override;
	void OnStart() override;
	void OnUpdate(float aDeltaTime) override;

	void SetState(PlayerState* aState);
	void StartForcedMoveTo(
		const CommonUtilities::Vector3<float>& aTargetPosition,
		float aSpeed,
		ForcedMoveCompleteCallback aOnComplete = {});
	void StopForcedMove();
	bool IsForcedMoveActive() const;
	void FaceDirection(const CommonUtilities::Vector3<float>& aDirection);

	void FireBullet();

private:
	void UpdateForcedMove(float aDeltaTime);
	void SetWalkAnimation(float aWeight);

	float mySpeed = 300.f;
	PlayerState* myState = nullptr;

	CommonUtilities::Vector3<float> myPosition;
	CommonUtilities::Vector3<float> myForcedMoveTarget = CommonUtilities::Vector3<float>::Zero;
	ForcedMoveCompleteCallback myForcedMoveCompleteCallback;
	float myForcedMoveSpeed = 600.0f;
	bool myForcedMoveActive = false;
};

