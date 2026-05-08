#include "SwitchComponent.h"
#include "Essentials.h"
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
	std::cout << myMessageID << std::endl;
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
	/*
	if (Essentials::globalInputManager->AnyInputPressed())
	{
		std::cout << myMessageID << std::endl;
		Toggle();
	}*/
}
