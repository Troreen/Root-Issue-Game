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

	void SetBullet(std::shared_ptr<GameObject> aBullet);
	GameObject& GetBullet();

private:
	float mySpeed = 300.f;
	PlayerState* myState;

	std::shared_ptr<GameObject> myBullet;
};

