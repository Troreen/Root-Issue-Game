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

	void Init(Tga::Engine& anEngine) override;
	void Initialize();

	void OnUpdate(float aDeltaTime) override;

	void Receive(const Message& aMSG) override;

	void Toggle();

	bool IsActivated() const { return myIsActivated; }

private:
	PostMaster* myPostMaster;
	bool myIsActivated = false;
	int myUniqueID;
	int myTypeID;

	float myAnimPercent;

	Tga::Vector3f myStartPos;
	Tga::Vector3f myEndPos;
};

