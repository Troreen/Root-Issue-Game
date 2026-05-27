#include "SplashScreen.h"
#include "Essentials.h"

#include <tge/engine.h>
#include <tge/graphics/DX11.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/texture/TextureManager.h>
#include "tge/drawers/SpriteDrawer.h"
#include <tge/math/Matrix4x4.h>
#include <Windows.h>

void SplashScreen::Init(CameraSystem& aCamera, const char* argv[])
{
	Essentials::globalEngine->SetClearColor({ 0,0,0,1 });
	myCameraSystem = &aCamera;
	argv;
	mySplashScreenState;
	Tga::Engine& engine = *Tga::Engine::GetInstance();
	Tga::SpriteSharedData myObj;
	if (engine.GetTextureManager().TryGetTexture("..//Source/Game/data/Assets/Art/2D/UI/TgaLogo.dds"))
	{
		myObj.myTexture = engine.GetTextureManager().GetTexture("..//Source/Game/data/Assets/Art/2D/UI/TgaLogo.dds");
		mySpriteList = myObj;
	}
	myCameraSystem->Init();
	mySceneName.clear();
	myCameraSystem->SetSceneName(mySceneName);
	mySpriteProperties.myColor.Set(1.0f, 1.0f, 1.0f, 0.0f);
}

eState SplashScreen::Update()
{
	myInputHandler.UpdateInput();
	for (int i = 0; i < static_cast<int>(Keys::OEM_CLEAR); i++)
	{	
		if (Essentials::globalInputManager.get()->IsKeyPressed(i))
		{
			return eState::eMainMenu;
		}
	}
	if (mySpriteProperties.myColor.a < 0.99f && !mySplashScreenState[0])
	{
		mySpriteProperties.myColor.a += 0.01f;
		return eState::COUNT;
	}
	else if (!mySplashScreenState[0])
	{
		mySpriteProperties.myColor.a = 1.0f;
		mySplashScreenState[0] = true;
		Sleep(1000);
		return eState::COUNT;
	}

	if (mySpriteProperties.myColor.a > 0.01f && !mySplashScreenState[1])
	{
		mySpriteProperties.myColor.a -= 0.01f;
		return eState::COUNT;
	}
	else if (!mySplashScreenState[1])
	{
		mySpriteProperties.myColor.a = 0.0f;
		mySplashScreenState[1] = true;
		Tga::Engine& engine = *Tga::Engine::GetInstance();
		Tga::SpriteSharedData myObj;
		if (engine.GetTextureManager().TryGetTexture("..//Source/Game/data/Assets/Art/2D/UI/P4G1_logo.dds"))
		{
			myObj.myTexture = engine.GetTextureManager().GetTexture("..//Source/Game/data/Assets/Art/2D/UI/P4G1_logo.dds");
			mySpriteList = myObj;
		}
		return eState::COUNT;
	}

	if (mySpriteProperties.myColor.a < 0.99f && !mySplashScreenState[2])
	{
		mySpriteProperties.myColor.a += 0.01f;
		return eState::COUNT;
	}
	else if (!mySplashScreenState[2])
	{
		mySpriteProperties.myColor.a = 1.0f;
		mySplashScreenState[2] = true;
		return eState::COUNT;
	}

	if (mySpriteProperties.myColor.a > 0.01f && !mySplashScreenState[3])
	{
		mySpriteProperties.myColor.a -= 0.01f;
		return eState::COUNT;
	}
	else if (!mySplashScreenState[3])
	{
		mySpriteProperties.myColor.a = 0.0f;
		mySplashScreenState[3] = true;
		Tga::Engine& engine = *Tga::Engine::GetInstance();
		Tga::SpriteSharedData myObj;
		if (engine.GetTextureManager().TryGetTexture("Sprites/FMOD Logo White - Transparent Background.dds"))
		{
			myObj.myTexture = engine.GetTextureManager().GetTexture("Sprites/FMOD Logo White - Transparent Background.dds");
			mySpriteList = myObj;
		}
		return eState::COUNT;
	}

	if (mySpriteProperties.myColor.a < 0.99f && !mySplashScreenState[4])
	{
		mySpriteProperties.myColor.a += 0.01f;
		return eState::COUNT;
	}
	else if (!mySplashScreenState[4])
	{
		mySpriteProperties.myColor.a = 1.0f;
		mySplashScreenState[4] = true;
		return eState::COUNT;
	}

	if (mySpriteProperties.myColor.a > 0.01f && !mySplashScreenState[5])
	{
		mySpriteProperties.myColor.a -= 0.01f;
		return eState::COUNT;
	}
	else if (!mySplashScreenState[5])
	{
		mySpriteProperties.myColor.a = 0.0f;
		mySplashScreenState[5] = true;
		return eState::COUNT;
	}
	return eState::eMainMenu;
}

void SplashScreen::Render()
{
	Tga::DX11::BackBuffer->SetAsActiveTarget();
	Tga::Engine& engine = *Tga::Engine::GetInstance();
	Tga::Vector2ui intResolution = engine.GetRenderSize();
	Tga::Vector2f resolution = { (float)intResolution.x, (float)intResolution.y };
	
	Tga::SpriteDrawer& spriteDrawer(engine.GetGraphicsEngine().GetSpriteDrawer());

	Tga::Vector2f SpriteSize = Tga::Vector2f(static_cast<float>(mySpriteList.myTexture->CalculateTextureSize().x), static_cast<float>(mySpriteList.myTexture->CalculateTextureSize().y));

	mySpriteProperties.myPosition = resolution * 0.5f;
	mySpriteProperties.mySize = (Tga::Vector2f{ 0.0003f, 0.0005f }) * (resolution * SpriteSize);

	mySpriteList.myTexture->CalculateTextureSize();
	spriteDrawer.Draw(mySpriteList, mySpriteProperties);
	Tga::DX11::BackBuffer->SetAsActiveTarget(Tga::DX11::DepthBuffer);
	RenderDefault();
}
