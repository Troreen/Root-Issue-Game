#include "DestructibleComponent.h"
#include "GameObject.h"
#include "AnimationGraphComponent.h"
#include "CapsuleColliderComponent.h"
#include <iostream>
void DesctructibleComponent::OnStart()
{
	mySave = false;
}

void DesctructibleComponent::Toggle()
{
	std::cout << "Die\n";
	GetOwner()->GetComponent<AnimationGraphComponent>()->SetFloatParameter("w_destroy", 1.f);
	GetOwner()->GetComponent<CapsuleColliderComponent>()->SetIsTrigger(true);
}

void DesctructibleComponent::Reset()
{
	if (!mySave)
	{
		GetOwner()->GetComponent<AnimationGraphComponent>()->SetFloatParameter("w_destroy", 0.f);
		GetOwner()->GetComponent<CapsuleColliderComponent>()->SetIsTrigger(false);
	}
}

void DesctructibleComponent::Save()
{
	mySave = GetOwner()->GetComponent<CapsuleColliderComponent>()->IsTrigger();
}
