#pragma once
#include "ScriptComponent.h"
#include "PostMaster.h"
#include "AnimationGraphComponent.h"
#include "GameObject.h"

enum class eTypeID
{
	eNothing,
	eDoor,
	eRootDoor,
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

	GameObject* myOwner = GetOwner();
	AnimationGraphComponent* myAnimationGraph = nullptr;

	PostMaster* myPostMaster;

	bool myIsActivated = false;
	int myUniqueID;
	int myTypeID;

	float myIdle;
	float myAnimPercent;

	Tga::Vector3f myStartPos;
	Tga::Vector3f myEndPos;

	void BindAnimationGraph() 
	{
		myOwner = GetOwner();
		if (myOwner->HasComponent<AnimationGraphComponent>())
		{
			myAnimationGraph = myOwner->GetComponent<AnimationGraphComponent>();
		}
	};
};

