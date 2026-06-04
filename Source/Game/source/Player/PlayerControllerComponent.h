#pragma once
#include "ScriptComponent.h"
#include "PlayerState.h"
#include "CommonUtilities/Vector.hpp"
#include "GameObject.h"
#include <functional>
#include <string>
#include <vector>
#include "AnimationEventListener.h"

struct SceneObjectData;

class PlayerControllerComponent : public ScriptComponent, public AnimationEventListener
{
public:
	using ForcedMoveCompleteCallback = std::function<void()>;
	using ScriptedAnimationCompleteCallback = std::function<void()>;

	PlayerControllerComponent(const SceneObjectData& aData);
	~PlayerControllerComponent() = default;

	void Reset() override;
	void Save() override;
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
	bool StartScriptedPickupAnimation(
		const std::string& aParameterName,
		float aDuration,
		ScriptedAnimationCompleteCallback aOnComplete = {});
	bool IsScriptedPickupAnimationActive() const;

	void FireBullet();
	void EnableGun(bool aEnable);

	bool HasGun() const;

	bool IsMoveInput();
	bool IsFireInput();

	void OnAnimationEvent(const AnimationEventContext& aEvent) override;

private:
	void UpdateForcedMove(float aDeltaTime);
	void UpdateScriptedPickupAnimation(float aDeltaTime);
	void StopScriptedPickupAnimation(bool aShouldComplete);
	void SetWalkAnimation(float aWeight);

	float mySpeed = 300.f;
	PlayerState* myState = nullptr;

	CommonUtilities::Vector3<float> myPosition;
	CommonUtilities::Vector3<float> myForcedMoveTarget = CommonUtilities::Vector3<float>::Zero;
	ForcedMoveCompleteCallback myForcedMoveCompleteCallback;
	ScriptedAnimationCompleteCallback myScriptedPickupCompleteCallback;
	std::string myScriptedPickupParameterName;
	float myScriptedPickupTimer = 0.0f;
	float myForcedMoveSpeed = 600.0f;
	bool myForcedMoveActive = false;
	bool myScriptedPickupActive = false;
	bool myColliderOffset = false;
	bool myHasGun = false;
};

