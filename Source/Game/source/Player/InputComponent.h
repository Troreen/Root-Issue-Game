#pragma once
#include "ScriptComponent.h"
#include <CommonUtilities/Vector.hpp>
#include <MouseDirectionComponent.h>

class InputComponent : public ScriptComponent
{
public:

	void OnStart() override;
	void OnUpdate(float aDeltaTimer) override;

	const CommonUtilities::Vector3<float>& GetTentativeDirection();
	const CommonUtilities::Vector3<float>& GetFacingDirection();
	const CommonUtilities::Vector3<float>& GetAimingDirection();

	bool HasInput();
	bool IsWalking();
	bool IsAttacking();
	bool IsCharging();

private:

	void UpdateWalk();
	void UpdateAim();

	bool myHasInput;
	bool myIsAttacking;
	bool myIsCharging;
	bool myIsWalking;

	Tga::InputManager* myInput = nullptr;

	CommonUtilities::Vector3<float> myTentativeDirection;
	CommonUtilities::Vector3<float> myFacingDirection;
	CommonUtilities::Vector3<float> myAimingDirection;
	CommonUtilities::Vector3<float> myForwardAxis;
	CommonUtilities::Vector3<float> myRightAxis;

	MouseDirectionComponent* myMouse = nullptr;
};