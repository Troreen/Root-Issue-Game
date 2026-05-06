#pragma once
#include "ScriptComponent.h"
#include "PlayerState.h"
#include "CommonUtilities/Vector.hpp"
#include "GameObject.h"
#include <vector>

class PlayerControllerComponent : public ScriptComponent
{
public:
	PlayerControllerComponent() = default;
	~PlayerControllerComponent() = default;

	void OnStart() override;
	void OnUpdate(float aDeltaTime) override;

	void SetState(PlayerState* aState);

private:
	float mySpeed = 300.f;
	PlayerState* myState;

	CommonUtilities::Vector3<float> myPosition;
};

