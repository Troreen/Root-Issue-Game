#pragma once
#include "ScriptComponent.h"
#include "GameObject.h"

class PickUpComponent : public ScriptComponent
{
public:
	PickUpComponent() = default;
	~PickUpComponent() = default;

	void OnUpdate(float aDeltaTime) override;

	bool IsTouching(const GameObject& aTargett);
};

