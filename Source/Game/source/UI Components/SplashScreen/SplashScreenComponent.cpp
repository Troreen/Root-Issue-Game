#include "SplashScreenComponent.h"
#include "WorldTransitionService.h"
#include "Essentials.h"


//SplashScreenComponent::SplashScreenComponent(const Tga::StringId& aCanvasName) : mySplashScreenAnimator((Tga::Sprite3DInstanceData*)nullptr, nullptr, 16, 32, std::vector<Animation2D>{})
//{
//	myCanvasName = aCanvasName;
//}

SplashScreenComponent::SplashScreenComponent() : mySplashScreenAnimator((Tga::Sprite3DInstanceData*)nullptr, nullptr, 16, 32, std::vector<Animation2D>{})
{

}

void SplashScreenComponent::Init(Tga::Engine& /*anEngine*/)
{
	Tga::Engine::GetInstance()->SetClearColor(Tga::Color(0, 0, 0, 1));
	myUICanvas.Init("SplashScreen", *Essentials::globalCanvasManager);
    mySplashScreens.push_back(myUICanvas.GetImage("TgaLogo"));
	mySplashScreensShownTime.push_back(1.0f);
	UIImage* groupLogo = myUICanvas.GetImage("GoopyGamesLogo");
    mySplashScreens.push_back(groupLogo);
	mySplashScreensShownTime.push_back(1.5f);
    mySplashScreens.push_back(myUICanvas.GetImage("FMODLogo"));
	mySplashScreensShownTime.push_back(0.5f);

	mySplashScreenAnimator = Animator2D(&groupLogo->instance, &groupLogo->shared, 2, 512, {Animation2D("Idle", { 1,2 }, true)});
	mySplashScreenAnimator.PlayAnimation2D("Idle");

	for (int i = 1; i < mySplashScreens.size(); i++)
	{
		mySplashScreens[i]->tint = Tga::Color(0.f, 0.f, 0.f, 0.f);
	}
	mySplashFadeTime = 0.75f;
}

void SplashScreenComponent::OnUpdate(float aDeltaTime)
{
	mySplashScreenAnimator.Update();
	mySplashScreenCurrentTime += aDeltaTime;

	if (mySplashScreenCurrentTime < mySplashFadeTime)
	{
		float colorValue = std::lerp(0.f, 1.f, mySplashScreenCurrentTime / mySplashFadeTime);
		mySplashScreens[mySplashScreenIndex]->tint = Tga::Color(colorValue, colorValue, colorValue, colorValue);
	}
	else if (mySplashScreenCurrentTime > mySplashFadeTime * 2 + mySplashScreensShownTime[mySplashScreenIndex])
	{
		mySplashScreenIndex++;
		if (mySplashScreenIndex < mySplashScreens.size())
		{
			mySplashScreenCurrentTime = 0.f;
			mySplashScreens[mySplashScreenIndex]->tint = Tga::Color(0.f, 0.f, 0.f, 0.f);
			return;
		}

		WorldTransitionService::RequestSceneTransition("Levels/TestScenes/MainMenuTest.tgs", "", 0);
		return;
	}
	else if (mySplashScreenCurrentTime > mySplashFadeTime + mySplashScreensShownTime[mySplashScreenIndex])
	{
		float colorValue = std::lerp(1.f, 0.f, (mySplashScreenCurrentTime - (mySplashFadeTime + mySplashScreensShownTime[mySplashScreenIndex])) / mySplashFadeTime);
		mySplashScreens[mySplashScreenIndex]->tint = Tga::Color(colorValue, colorValue, colorValue, colorValue);
	}
}

void SplashScreenComponent::Render()
{
	mySplashScreenAnimator.Render();
}
