#pragma once
#include "ScriptComponent.h"
class CheckpointComponent : public ScriptComponent
{
public:

	void Toggle();

private:

	bool myIsActive = false;
};