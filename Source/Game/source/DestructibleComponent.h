#pragma once
#include "ScriptComponent.h"
#include "DamageableComponent.h"

class DesctructibleComponent : public ScriptComponent
{
public:

	void OnStart() override;
	void Toggle();

	void Reset() override;
	void Save() override;

private:

	bool mySave;
};