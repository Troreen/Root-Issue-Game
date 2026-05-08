#pragma once
#include "ScriptComponent.h"
#include "PostMaster.h"
class ToggleComponent : public Subscriber, public ScriptComponent
{
public:
	ToggleComponent() = default;
	explicit ToggleComponent(int anID);
	~ToggleComponent();

	void Receive(const Message& aMSG) override;

	void Toggle();

	bool IsActivated() const { return myIsActivated; }

private:
	PostMaster* myPostMaster;
	bool myIsActivated = false;
	int myUniqueID;
};

