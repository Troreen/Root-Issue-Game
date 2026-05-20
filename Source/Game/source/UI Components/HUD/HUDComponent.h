#pragma once
#include "ScriptComponent.h"
#include "UICanvas.h"

class HUDComponent : public ScriptComponent
{
public:

	HUDComponent();

	void Init(Tga::Engine& anEngine) override;
	void OnUpdate(float aDeltaTime) override;

private:

	UICanvas myUICanvas;
};

