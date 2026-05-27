#include "HUDComponent.h"
#include "Essentials.h"
#include "DamageableComponent.h"
#include "GameObject.h"

HUDComponent::HUDComponent() = default;

void HUDComponent::Init(Tga::Engine& /*anEngine*/)
{
	myUICanvas.Init("HUD", *Essentials::globalCanvasManager);

	myHealthComponent = GetOwner()->GetComponent<DamageableComponent>();

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


