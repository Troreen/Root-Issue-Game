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
	void Render() override;

	void SetBullet(std::unique_ptr<GameObject> aBullet);
	void SetState(PlayerState* aState);

private:
	float mySpeed = 300.f;
	PlayerState* myState;

	CommonUtilities::Vector3<float> myCameraOffset;
	CommonUtilities::Vector3<float> myPosition;
	CommonUtilities::Quaternion<float> myCameraRotation;
	std::vector<std::unique_ptr<GameObject>> myBullets;
};

