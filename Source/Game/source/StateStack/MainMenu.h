#pragma once
#include "State.hpp"

class MainMenu : public State
{
public:
	MainMenu() = default;

	void Init(CameraSystem& aCamera, const char* argv[]) override;
	eState Update() override;
	void Render() override;
};