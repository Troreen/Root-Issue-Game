#include "GunUpgradeComponent.h"
#include "GameObject.h"
#include "Essentials/Essentials.h"
#include "PlayerControllerComponent.h"
#include "SceneObjectData.h"

void GunUpgradeComponent::OnStart()
{
	myCollider = GetOwner()->GetComponent<CapsuleColliderComponent>();
}

void GunUpgradeComponent::OnUpdate(float)
{
	if (myCollider->IsInside())
	{
		Essentials::GetPlayer()->GetComponent<PlayerControllerComponent>()->EnableGun(true);

		GetOwner()->SetActive(false);
	}
}
