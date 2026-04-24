#pragma once
#include "ScriptComponent.h"
#include "CommonUtilities/Vector.hpp"

class BulletComponent : public ScriptComponent
{
public:

	void OnUpdate(float aDeltaTime) override;

	void SetSpeedDirectionPosition(float aSpeed, CommonUtilities::Vector3<float> aDirection, CommonUtilities::Vector3<float> aPosition);
private:

	float myLifeTimer;
	float mySpeed;
	CommonUtilities::Vector3<float> myDirection;
};

