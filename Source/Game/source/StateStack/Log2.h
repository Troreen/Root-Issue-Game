#pragma once
#include "State.hpp"
#include "UICanvas.h"
#include "tge/text/text.h"
#include "RandomFloat.h"
#include <bitset>

class LogTransitionTwo : public State
{
public:
	LogTransitionTwo() = default;

	void Init(CameraSystem& aCamera, const char* argv[]) override;
	eState Update() override;
	void Render() override;

private:
	void UpdateTextRes();
	UICanvas myUICanvas;
	float myCounter;
	float myNextCharTimer;
	int mySize;

	std::bitset<1> myWaits;

	Tga::Text myText;
};