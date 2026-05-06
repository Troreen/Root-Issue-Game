#pragma once
#include <tge/math/Vector.h>
#include <tge/input/InputManager.h>
#include "CommonUtilities/Quaternion.hpp"
#include "CommonUtilities/Vector.hpp"

#include "ScriptComponent.h"

class MouseDirectionComponent : public ScriptComponent
{
public:

	void OnStart() override;
	void OnUpdate(float aDeltaTimer) override;
	const Tga::Vector2f& GetWorldDirection();

private:
	float myCameraAngle;
	float myCameraDownAngleScalar;

	Tga::Vector2f myResolution;
	Tga::Vector2f myMousePosition;
	Tga::Vector2f myWorldDirection;
	Tga::InputManager* myInput;

	CommonUtilities::Vector3<float> myOffset;
	CommonUtilities::Vector3<float> myPosition;
	CommonUtilities::Quaternion<float> myCameraRotation;
};