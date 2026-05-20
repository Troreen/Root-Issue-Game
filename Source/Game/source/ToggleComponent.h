#pragma once
#include "ScriptComponent.h"
#include "PostMaster.h"

enum class eTypeID
{
	eNothing,
	eDoor,
	COUNT
};

class ToggleComponent : public Subscriber, public ScriptComponent
{
public:
	ToggleComponent() = default;
	explicit ToggleComponent(int anID, bool aActive, int aTypeID);
	~ToggleComponent();

	void Initialize();

	void Receive(const Message& aMSG) override;

	void Toggle();

	bool IsActivated() const { return myIsActivated; }

private:
	PostMaster* myPostMaster;
	bool myIsActivated = false;
	int myUniqueID;
	int myTypeID;
};

