#pragma once

#include "ScriptComponent.h"
#include "PostMaster.h"
#include "Essentials.h"
#include <string>

class SwitchTriggerComponent final : public Subscriber, public ScriptComponent
{
public:
	SwitchTriggerComponent() = default;
	explicit SwitchTriggerComponent(int anUniqueID, bool anOnceTrigger);

protected:
	void OnUpdate(float aDeltaTime) override;

	void Receive(const Message& aMSG) override;

private:
	bool myWasInside = false;
	bool myHasTriggered = false;
	int myUniqueID;

	bool myOnceTrigger;
	bool myIsActivatedOnlyOnce;

	int myTriggerSize;

	MessageType myType;
	PostMaster* myPostMaster;
};