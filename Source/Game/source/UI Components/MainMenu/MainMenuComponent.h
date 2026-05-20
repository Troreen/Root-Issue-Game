#pragma once
#include "ScriptComponent.h"
#include "UICanvas.h"

class MainMenuComponent : public ScriptComponent
{

public:

	MainMenuComponent() = default;

	void Init(Tga::Engine& anEngine) override;
	void OnUpdate(float aDeltaTime) override;

private:

	UICanvas myUICanvas;
	bool myIsOpen;
};

