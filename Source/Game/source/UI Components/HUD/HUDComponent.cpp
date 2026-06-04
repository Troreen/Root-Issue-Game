#include "HUDComponent.h"
#include "Essentials.h"
#include "DamageableComponent.h"
#include "GameObject.h"
#include "PlayerControllerComponent.h"

HUDComponent::HUDComponent() = default;

void HUDComponent::Init(Tga::Engine& /*anEngine*/)
{
	myUICanvas.Init("HUD", *Essentials::globalCanvasManager);

	myHealthComponent = GetOwner()->GetComponent<DamageableComponent>();

	myPlayerControllerComponent = GetOwner()->GetComponent<PlayerControllerComponent>();


	if (!myPlayerControllerComponent)
	{
		std::cout << "HUD: No controller for the player - Player\n";
		return;
	}

	if (!myHealthComponent)
	{
		std::cout << "HUD: Health no worky - Player\n";
		return;
	}

	myPlayerMaxHealth = myHealthComponent->GetMaxHealth();
	myPlayerCurrentHealth = myPlayerMaxHealth;

}

void HUDComponent::OnUpdate(float /*aDeltaTime*/)
{
	if (!myHealthComponent)
	{
		return;
	}

	if (Essentials::globalSceneManager->GetCurrentScene() == "Levels/HUB_00.tgs")
	{
		ShowTutorial(true);
	}
	else
	{
		ShowTutorial(false);
	}

	if (myShowGunTutorial)
	{
		UpdateGunTutorial();
	}

	UpdateHealthHUD();

	CheckAndSetIfPlayerHasGun();
}

void HUDComponent::ShowTutorial(bool aTutorial)
{
	if (aTutorial)
	{
		if (Essentials::globalInputManager->IsConnected())
		{
			myUICanvas.SetIsHidden("XBOXTutorial", !aTutorial);
			myUICanvas.SetIsHidden("PCTutorial", aTutorial);
		}
		else
		{
			myUICanvas.SetIsHidden("PCTutorial", !aTutorial);
			myUICanvas.SetIsHidden("XBOXTutorial", aTutorial);
		}
	}
	else
	{
		myUICanvas.SetIsHidden("PCTutorial", !aTutorial);
		myUICanvas.SetIsHidden("XBOXTutorial", !aTutorial);
	}
}

void HUDComponent::UpdateHealthHUD()
{
	myPlayerCurrentHealth = myHealthComponent->GetCurrentHealth();

	switch (myPlayerCurrentHealth)
	{
	case 0:
		myUICanvas.SetIsHidden("Health1", true);
		myUICanvas.SetIsHidden("Health2", true);
		myUICanvas.SetIsHidden("Health3", true);
		myUICanvas.SetIsHidden("Health4", true);
		break;
	case 1:
		myUICanvas.SetIsHidden("Health1", false);
		myUICanvas.SetIsHidden("Health2", true);
		myUICanvas.SetIsHidden("Health3", true);
		myUICanvas.SetIsHidden("Health4", true);
		break;
	case 2:
		myUICanvas.SetIsHidden("Health1", false);
		myUICanvas.SetIsHidden("Health2", false);
		myUICanvas.SetIsHidden("Health3", true);
		myUICanvas.SetIsHidden("Health4", true);
		break;
	case 3:
		myUICanvas.SetIsHidden("Health1", false);
		myUICanvas.SetIsHidden("Health2", false);
		myUICanvas.SetIsHidden("Health3", false);
		myUICanvas.SetIsHidden("Health4", true);
		break;
	case 4:
		myUICanvas.SetIsHidden("Health1", false);
		myUICanvas.SetIsHidden("Health2", false);
		myUICanvas.SetIsHidden("Health3", false);
		myUICanvas.SetIsHidden("Health4", false);
		break;
	default:
		myUICanvas.SetIsHidden("Health1", false);
		myUICanvas.SetIsHidden("Health2", false);
		myUICanvas.SetIsHidden("Health3", false);
		myUICanvas.SetIsHidden("Health4", false);
		break;
	}
}

void HUDComponent::CheckAndSetIfPlayerHasGun()
{
	if (myPlayerControllerComponent->HasGun())
	{
		if (Essentials::FirstTimeGunPickup)
		{
			Essentials::FirstTimeGunPickup = false;
			myShowGunTutorial = true;
			myTutorialVisibleTimer = 5.0f;
			ShowGunTutorial(true);
		}

		myUICanvas.SetIsHidden("GunImageEmpty", true);
		myUICanvas.SetIsHidden("GunImage", false);
	}
	else
	{
		myUICanvas.SetIsHidden("GunImageEmpty", false);
		myUICanvas.SetIsHidden("GunImage", true);
	}
}

void HUDComponent::ShowGunTutorial(bool aGunTutorial)
{
	if (aGunTutorial)
	{
		if (Essentials::globalInputManager->IsConnected())
		{
			myUICanvas.SetIsHidden("XBOXGunTutorial", !aGunTutorial);
			myUICanvas.SetIsHidden("PCGunTutorial", aGunTutorial);

		}
		else
		{
			myUICanvas.SetIsHidden("XBOXGunTutorial", aGunTutorial);
			myUICanvas.SetIsHidden("PCGunTutorial", !aGunTutorial);
		}
	}
	else
	{
		myUICanvas.SetIsHidden("PCGunTutorial", !aGunTutorial);
		myUICanvas.SetIsHidden("XBOXGunTutorial", !aGunTutorial);
	}
}

void HUDComponent::UpdateGunTutorial()
{
	myTutorialVisibleTimer -= Essentials::GetDeltaTime();

	if (myTutorialVisibleTimer <= 0.0f)
	{
		myShowGunTutorial = false;
		ShowGunTutorial(false);
	}
}


