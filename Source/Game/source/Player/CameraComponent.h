#pragma once
#include "ScriptComponent.h"
#include "CommonUtilities/Vector.hpp"
#include "CommonUtilities/Quaternion.hpp"

class CameraComponent : public ScriptComponent
{
public:

	void OnStart() override;
	void OnUpdate(float aDeltaTime) override;

private:

	CommonUtilities::Vector3<float> myOffset;
	CommonUtilities::Vector3<float> myPosition;
	CommonUtilities::Quaternion<float> myCameraRotation;
};