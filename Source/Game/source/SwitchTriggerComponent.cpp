#include "SwitchTriggerComponent.h"
#include "WorldTriggerHelpers.h"

SwitchTriggerComponent::SwitchTriggerComponent(int anUniqueID, bool anOnceTrigger) : myUniqueID(anUniqueID), myType(MessageType::ActivateSwitch), myPostMaster(Essentials::globalPostMaster.get()), myIsActivatedOnlyOnce(anOnceTrigger)
{
	myOnceTrigger = true;
	myTriggerSize = 0;
}

void SwitchTriggerComponent::OnUpdate(float /*aDeltaTime*/)
{
	if (myTriggerSize > 0)
	{
		return;
	}
	GameObject* owner = GetOwner();
	if (!owner)
	{
		return;
	}
	const bool isInside = WorldTriggerHelpers::IsTriggerInside(*owner);
	const bool entered = isInside && !myWasInside;
	if (isInside)
	{
		if (myOnceTrigger)
		{
			myTriggerSize += static_cast<int>(myIsActivatedOnlyOnce);
			myOnceTrigger = false;
			Message a;
			a.myInt = myUniqueID;
			a.myMessageType = myType;
			a.mySender = this->GetOwner();
			myPostMaster->SendMsg(a);
			return;
		}
	}
	if (!isInside)
	{
		myWasInside = false;
		myOnceTrigger = true;
		return;
	}

	myWasInside = true;
	if (!entered || myHasTriggered)
	{
		return;
	}
}

void SwitchTriggerComponent::Receive(const Message& aMSG)
{
	aMSG;
}