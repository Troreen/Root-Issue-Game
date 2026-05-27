#pragma once

#include "ScriptComponent.h"
#include "GameObject.h"

class GeneratorComponent : public ScriptComponent
{
public:

	GeneratorComponent() = default;
	~GeneratorComponent() = default;

	void OnUpdate(float aDeltaTime) override;

private:

	float myTimer = 0;
	float myEmissionInterval = 1.f;
};

