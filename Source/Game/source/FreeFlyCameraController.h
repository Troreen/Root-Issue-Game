#pragma once

#include "Camera3D.hpp"
#include "InputHandler.h"

class FreeFlyCameraController
{
public:
	using CUCamera3Df = CommonUtilities::Camera3D<float>;

	FreeFlyCameraController();

	void Init(CommonUtilities::InputHandler& anInputHandler, CUCamera3Df& aCamera);
	void Update(float aTimeDelta);
	void ResetMouseLookAnchor();

	void SetMoveSpeed(float aMoveSpeed);
	void SetLookSensitivity(float aLookSensitivity);
	float GetMoveSpeed() const;
	float GetLookSensitivity() const;

private:
	CommonUtilities::InputHandler* myInputHandler;
	CUCamera3Df* myCamera;

	float myMoveSpeed;
	float myLookSensitivity;
	float myYawRadians;
	float myPitchRadians;
	float myMaxPitchRadians;
	bool myHasMouseLookAnchor;
};