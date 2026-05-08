#pragma once
#include "ScriptComponent.h"
#include "CommonUtilities/Vector.hpp"
#include <vector>
#include <memory>
#include "GameObject.h"


class BulletComponent : public ScriptComponent
{
public:

	void SetTransform(CommonUtilities::Transform<float> aTransform);
	void OnUpdate(float aDeltaTime) override;

private:

	float myTimer;
	float mySpeed;

	CommonUtilities::Transform<float> myTransform;
};

