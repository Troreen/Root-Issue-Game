#include "SwitchComponent.h"
#include "Essentials.h"
#include "AnimationGraphComponent.h"
#include <iostream>

SwitchComponent::SwitchComponent(int anID)
	: myType(MessageType::ActivateSwitch), myMessageID(anID), myPostMaster(Essentials::globalPostMaster.get())
{

}

SwitchComponent::~SwitchComponent()
{
}

void SwitchComponent::Receive(const Message& aMSG)
{
	aMSG;
}

void SwitchComponent::Toggle()
{
	if (GetOwner()->HasComponent<AnimationGraphComponent>())
	{
		GetOwner()->GetComponent<AnimationGraphComponent>()->SetFloatParameter("w_activated", 1.0f);
	}
	myIsActivated = !myIsActivated;
	Message a;
	a.myInt = myMessageID;
	a.myMessageType = myType;
	a.mySender = this->GetOwner();
	myPostMaster->SendMsg(a);
}

void SwitchComponent::OnUpdate(float aDeltaTime)
{
	aDeltaTime;
}
