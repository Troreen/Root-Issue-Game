#pragma once
#include "State.hpp"
#include "UICanvas.h"

class MainMenu : public State
{
public:

	MainMenu() = default;

	void Init(CameraSystem& aCamera, const char* argv[]) override;
	eState Update() override;
	void Render() override;

private:

	void UpdateMainMenuUI();

	UICanvas myUICanvas;
	bool myIsOpen;
	bool myStartGame;
};