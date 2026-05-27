#include "Log.h"

void LogTransition::Init(CameraSystem& aCamera, const char* argv[])
{
	aCamera;
	argv;
}

eState LogTransition::Update()
{
	return eState::COUNT;
}

void LogTransition::Render()
{
}