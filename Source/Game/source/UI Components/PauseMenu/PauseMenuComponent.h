#pragma once
#include "ScriptComponent.h"
#include "UICanvas.h"


class PauseMenuComponent : public ScriptComponent
{
public:

	PauseMenuComponent();

	void Init(Tga::Engine& anEngine) override;
	void OnUpdate(float aDeltaTime) override;

	static bool IsOpen();

	bool ReturnToMainMenu() const;

private:

	UICanvas myUICanvas;
	static inline bool myIsOpen;
	bool myReturnToMainMenu;
};

