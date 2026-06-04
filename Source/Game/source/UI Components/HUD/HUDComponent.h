#pragma once
#include "ScriptComponent.h"
#include "UICanvas.h"

class DamageableComponent;
class PlayerControllerComponent;

class HUDComponent : public ScriptComponent
{
public:

	HUDComponent();

	void Init(Tga::Engine& anEngine) override;
	void OnUpdate(float aDeltaTime) override;

private:

	void ShowTutorial(bool aTutorial);
	void UpdateHealthHUD();
	void CheckAndSetIfPlayerHasGun();
	void ShowGunTutorial(bool aGunTutorial);
	void UpdateGunTutorial();

	UICanvas myUICanvas;

	DamageableComponent* myHealthComponent = nullptr;
	PlayerControllerComponent* myPlayerControllerComponent = nullptr;

	int myPlayerMaxHealth;
	int myPlayerCurrentHealth;

	float myTutorialVisibleTimer = 2.0f;

	bool myFirstTimeGunPickup = false;
	bool myShowGunTutorial = false;
};