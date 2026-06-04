#pragma once
#include "State.hpp"
#include <tge/sprite/sprite.h>
#include <tge/videoplayer/video.h>

class Outro : public State
{
public:
	Outro() = default;

	void Init(CameraSystem& aCamera, const char* argv[]) override;
	eState Update() override;
	void Render() override;

private:

	std::shared_ptr<Tga::Video> myVideo;
	Tga::Sprite2DInstanceData myVideoSprite;
	Tga::SpriteSharedData myVideoSpriteData;
};