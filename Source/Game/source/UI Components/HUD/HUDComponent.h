#pragma once
#include "ScriptComponent.h"
#include "UICanvas.h"

class DamageableComponent;

class HUDComponent : public ScriptComponent
{
public:

	HUDComponent();

	void Init(Tga::Engine& anEngine) override;
	void OnUpdate(float aDeltaTime) override;

private:

	UICanvas myUICanvas;

	DamageableComponent* myHealthComponent = nullptr;

	int myPlayerMaxHealth;
	int myPlayerCurrentHealth;
};

