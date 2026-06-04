#include "ToggleComponent.h"
#include "Essentials.h"
#include <iostream>
#include "GameObject.h"
#include "BoxColliderComponent.h"
#include "ObbColliderComponent.h"
#include "AnimationGraphComponent.h"
#include "AnimatedMeshComponent.h"
#include "tge/engine.h"

ToggleComponent::ToggleComponent(int anID, bool aActive, int anTypeID) : myUniqueID(anID), myIsActivated(aActive), myTypeID(anTypeID), myPostMaster(Essentials::globalPostMaster.get())
{
	myPostMaster->Subscribe(MessageType::ActivateSwitch, this);
}

ToggleComponent::~ToggleComponent()
{
	myPostMaster->Unsubscribe(MessageType::ActivateSwitch, this);
}

void ToggleComponent::Init(Tga::Engine& anEngine)
{
	(void)anEngine;
	Initialize();
}

void ToggleComponent::Initialize()
{
	BindAnimationGraph();
	if (myIsActivated == true)
	{
		myIsActivated = !myIsActivated;
		myStartPos = Tga::Vector3f(0.f, 0.f, 0.f);
		myEndPos = Tga::Vector3f(0.f,  -400.0f, 0.f);
		Toggle();
	}
	else
	{
		myStartPos = Tga::Vector3f(0.f, 0.f, 0.f);
		myEndPos = Tga::Vector3f(0.f, -400.0f, 0.f);
		myIdle = -1.0f;
	}
}

void ToggleComponent::OnUpdate(float /*aDeltaTime*/)
{
	switch (static_cast<eTypeID>(myTypeID))
	{
	case eTypeID::eDoor:
		if (myOwner->HasComponent<ObbColliderComponent>())
		{
			myOwner->GetComponent<ObbColliderComponent>()->SetOffset(Tga::Vector3f::Lerp(myStartPos, myEndPos, myIsActivated));
		}
		if (myOwner->HasComponent<AnimationGraphComponent>())
		{
			myOwner->GetComponent<AnimationGraphComponent>()->SetFloatParameter("w_Active", myIdle);
		}
		break;
	default:
		break;
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
	if (myIsActivated)
	{
		switch (static_cast<eTypeID>(myTypeID))
		{
		case eTypeID::eNothing:
			break;
		case eTypeID::eDoor:
			if (myAnimationGraph)
			{
				if (myOwner->GetObjDefinition() == "HubDoor_01")
				{
					if (!Essentials::globalAudioManager.get()->IsEventPlaying(SoundID::eRootDoor))
					{
						Essentials::globalAudioManager.get()->PlaySFXAtLocation(SoundID::eRootDoor, (GetOwner()->GetTransform().GetPosition().ToTga() - Essentials::GetPlayer()->GetTransform().GetPosition().ToTga()));
					}
				}
				myAnimationGraph->SetEnabled(true);
				myIdle = 1.0f;
				myOwner->GetComponent<AnimationGraphComponent>()->SetFloatParameter("w_Active", myIdle);
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
			if (myAnimationGraph)
			{
				myAnimationGraph->SetEnabled(true);
				myIdle = -1.0f;
				myOwner->GetComponent<AnimationGraphComponent>()->SetFloatParameter("w_Active", myIdle);
			}
			break;
		default:
			break;
		}
	}
}