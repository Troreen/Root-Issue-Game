#pragma once
#include "BoxColliderComponent.h"
#include "GameObject.h"
#include "ToggleComponent.h"
#include "ScriptComponent.h"
#include "PostMaster.h"
#include "Essentials.h"
#include <vector>

class SwitchComponent : public Subscriber, public ScriptComponent
{
public:
	SwitchComponent() = default;
	explicit SwitchComponent(int anID);

	~SwitchComponent() override;

	void Receive(const Message& aMSG) override;

	void Toggle();

	void OnUpdate(float aDeltaTime) override;


private:
	bool myIsActivated = false;
	MessageType myType;
	int myMessageID;
	PostMaster* myPostMaster;
};