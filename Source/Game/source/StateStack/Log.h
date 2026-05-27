#pragma once
#include "State.hpp"

class LogTransition : public State
{
public:
	LogTransition() = default;

	void Init(CameraSystem& aCamera, const char* argv[]) override;
	eState Update() override;
	void Render() override;

private:

};