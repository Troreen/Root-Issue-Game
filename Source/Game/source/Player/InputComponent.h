#pragma once
#include "ScriptComponent.h"
#include <CommonUtilities/Vector.hpp>

class InputComponent : public ScriptComponent
{
public:

	void OnStart() override;
	void OnUpdate(float aDeltaTimer) override;

	const CommonUtilities::Vector3<float>& GetFacingDirection();

	bool HasInput();
	//bool IsAttacking();
	//bool IsCharging();

private:

	void UpdateWalk();

	bool myHasInput;
	bool myIsAttacking;
	bool myIsCharging;
	bool myIsWalking;

	CommonUtilities::Vector3<float> myTentativeDirection;
	CommonUtilities::Vector3<float> myFacingDirection;
	CommonUtilities::Vector3<float> myForwardAxis;
	CommonUtilities::Vector3<float> myRightAxis;
};