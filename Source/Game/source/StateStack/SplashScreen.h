#pragma once
#include "State.hpp"
#include <bitset>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/sprite/sprite.h>

class SplashScreen : public State
{
public:
	SplashScreen() = default;
	void Init(CameraSystem& aCamera, const char* argv[]) override;
	eState Update() override;
	void Render() override;
private:
	std::bitset<6> mySplashScreenState;

	Tga::Sprite2DInstanceData mySpriteProperties;
	Tga::SpriteSharedData mySpriteList;
};