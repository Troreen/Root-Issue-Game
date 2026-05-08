#pragma once
#include "ScriptComponent.h"
#include "PostMaster.h"
#include "SceneObjectData.h"

class ResetComponent : public ScriptComponent, public Subscriber
{
public:
	ResetComponent() = delete;
	ResetComponent(const SceneObjectData& aResetData);
	~ResetComponent();

	void Receive(const Message& aMsg) override;

private:
	SceneObjectData myResetData;
};