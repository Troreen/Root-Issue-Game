#pragma once
#include "State.hpp"

class Options : public State
{
public:
	Options() = default;

	void Init(CameraSystem& aCamera, const char* argv[]) override;
	eState Update() override;
	void Render() override;
};