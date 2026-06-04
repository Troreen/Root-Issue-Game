#include "Outro.h"
#include "Essentials.h"

#include <tge/drawers/SpriteDrawer.h>
#include <tge/engine.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/settings/settings.h>

void Outro::Init(CameraSystem& aCamera, const char* argv[])
{
	myCameraSystem = &aCamera;
	argv;
	myVideo = std::make_shared<Tga::Video>();
	myVideo->Init(Tga::Settings::ResolveAssetPath("animations/MP4/A_CS_Outro.mp4").c_str());
	myVideo->Play();
	myVideoSpriteData.myTexture = myVideo->GetTexture();

	Tga::Vector2ui res = Tga::Engine::GetInstance()->GetRenderSize();
	Tga::Vector2f resf = Tga::Vector2f(static_cast<float>(res.x), static_cast<float>(res.y));
	Tga::Vector2f resolution = { static_cast<float>(Tga::Engine::GetInstance()->GetRenderSize().x), static_cast<float>(Tga::Engine::GetInstance()->GetRenderSize().y) };

	Tga::Vector2f videoRes = Tga::Vector2f((static_cast<float>(res.x)), static_cast<float>(res.y));

	myVideoSprite.myPivot = { 0.5f,0.5f };
	myVideoSprite.mySize = Tga::Vector2f(1.0f, 1.5f) * resolution;
	myVideoSprite.myPosition.y = resolution.y * 0.15f;
	myVideoSprite.myPosition.x = resolution.x * (0.5f + ((0.63f - 0.5f) / 4));
	Essentials::globalAudioManager->PlayMusic(SoundID::eOutroSFX);
}
eState Outro::Update()
{
	Essentials::globalAudioManager->Update(Essentials::GetDeltaTime());
	myVideo->Update(Essentials::GetDeltaTime());

	myInputHandler.UpdateInput();
	for (int i = 0; i < static_cast<int>(Keys::OEM_CLEAR); i++)
	{
		if (Essentials::globalInputManager.get()->IsKeyPressed(i))
		{
			Essentials::globalAudioManager->StopAllEvents();
			return eState::ePopState;
		}
	}

	if (myVideo->GetStatus() == Tga::VideoStatus::ReachedEnd)
	{
		return eState::ePopState;
	}

	return eState::COUNT;
}
void Outro::Render()
{
	auto& engine = *Tga::Engine::GetInstance();
	engine.SetClearColor({ 0,0,0,1 });
	Tga::SpriteDrawer& aSpriteDrawer(engine.GetGraphicsEngine().GetSpriteDrawer());
	aSpriteDrawer.Draw(myVideoSpriteData, myVideoSprite);
}