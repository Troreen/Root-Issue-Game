#include "ToggleComponent.h"
#include "Essentials.h"
#include <iostream>
#include "GameObject.h"
#include "BoxColliderComponent.h"
#include "tge/engine.h"

ToggleComponent::ToggleComponent(int anID, bool aActive, int anTypeID) : myUniqueID(anID), myIsActivated(aActive), myTypeID(anTypeID), myPostMaster(Essentials::globalPostMaster.get())
{
	myPostMaster->Subscribe(MessageType::ActivateSwitch, this);
}

ToggleComponent::~ToggleComponent()
{
	myPostMaster->Unsubscribe(MessageType::ActivateSwitch, this);
}

void ToggleComponent::Initialize()
{
	if (myIsActivated == true)
	{
		std::cout << "TOGGLE!" << std::endl;
		myIsActivated = !myIsActivated;
		Toggle();
	}
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
	auto& engine = *Tga::Engine::GetInstance();
	auto& transform = GetOwner()->GetTransform();
	if (myIsActivated)
	{
		switch (static_cast<eTypeID>(myTypeID))
		{
		case eTypeID::eNothing:
			break;
		case eTypeID::eDoor:
			this->GetOwner()->GetTransform().SetPosition(Tga::Vector3f(transform.GetPosition().x, transform.GetPosition().y - 400.0f, transform.GetPosition().z));
			if (GetOwner()->HasComponent<BoxColliderComponent>())
			{
				GetOwner()->GetComponent<BoxColliderComponent>()->Init(engine);
			}
			break;
		default:
			break;
		}
	}
	else
	{
		switch (static_cast<eTypeID>(myTypeID))
		{
		case eTypeID::eNothing:
			break;
		case eTypeID::eDoor:
			transform.SetPosition(Tga::Vector3f(transform.GetPosition().x, transform.GetPosition().y + 400.0f, transform.GetPosition().z));
			if (GetOwner()->HasComponent<BoxColliderComponent>())
			{
				GetOwner()->GetComponent<BoxColliderComponent>()->Init(engine);
			}
			break;
		default:
			break;
		}
	}
}