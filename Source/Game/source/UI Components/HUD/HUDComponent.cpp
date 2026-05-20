#include "HUDComponent.h"
#include "Essentials.h"

HUDComponent::HUDComponent() = default;

void HUDComponent::Init(Tga::Engine& /*anEngine*/)
{
	myUICanvas.Init("HUD", *Essentials::globalCanvasManager);
}

void HUDComponent::OnUpdate(float /*aDeltaTime*/)
{
}


