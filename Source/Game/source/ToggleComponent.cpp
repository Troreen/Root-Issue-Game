#include "ToggleComponent.h"
#include "Essentials.h"
#include <iostream>
#include "GameObject.h"

ToggleComponent::ToggleComponent(int anID): myUniqueID(anID), myPostMaster(Essentials::globalPostMaster.get())
{
	myPostMaster->Subscribe(MessageType::ActivateSwitch, this);
}

ToggleComponent::~ToggleComponent()
{
	/*myPostMaster->Unsubscribe(MessageType::ActivateSwitch, this);*/
}

void ToggleComponent::Receive(const Message& aMSG)
{
	switch (aMSG.myMessageType)
	{
	case MessageType::ActivateSwitch:
		if (aMSG.myInt == myUniqueID)
		Toggle();
		break;
	default:
		break;
	}
}

void ToggleComponent::Toggle()
{
	myIsActivated = !myIsActivated;
	std::cout << GetOwner() << " toggled to " << (myIsActivated ? "ON" : "OFF") << std::endl;
	auto& transform = GetOwner()->GetTransform();
	if (myIsActivated)
	{
		if (GetOwner()->GetObjDefinition() == "HubDoor_01")
		{
			transform.SetPosition(Tga::Vector3f(transform.GetPosition().x, transform.GetPosition().y + 150.0f, transform.GetPosition().z));
		}
	}
	else
	{
		if (GetOwner()->GetObjDefinition() == "HubDoor_01")
		{
			transform.SetPosition(Tga::Vector3f(transform.GetPosition().x, transform.GetPosition().y - 150.0f, transform.GetPosition().z));
		}
	}
}