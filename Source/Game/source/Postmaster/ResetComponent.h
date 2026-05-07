#pragma once
#include "ScriptComponent.h"
#include "PostMaster.h"

class ResetComponent : public ScriptComponent, public Subscriber
{
public:
	ResetComponent();
	~ResetComponent();

	void Receive(const Message& aMsg) override;

private:
	std::vector<ScriptComponent> mySavedComponents;
};