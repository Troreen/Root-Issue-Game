//#include "SplashScreenBehaviour.h"
//#include "Essentials.h"
//
//SplashScreenBehaviour::SplashScreenBehaviour() : mySplashScreenAnimator((Tga::Sprite3DInstanceData*)nullptr, nullptr, 16, 32, std::vector<Animation2D>{})
//{
//}
//
//void SplashScreenBehaviour::Init(std::vector<Tga::ScenePropertyDefinition>& aSceneObjectProperties, std::shared_ptr<Tga::SceneObject> aSceneObject)
//{
//	BehaviourBase::Init(aSceneObjectProperties, aSceneObject);
//	Tga::Engine::GetInstance()->SetClearColor(Tga::Color(0, 0, 0, 1));
//	Tga::StringId canvasPath = GetBasePropertyOr<Tga::StringId>("CanvasName");
//	myUICanvas.Init(canvasPath, *Essentials::globalCanvasManager);
//    mySplashScreens.push_back(myUICanvas.GetImage("TgaLogo"));
//	mySplashScreensShownTime.push_back(1.0f);
//	UIImage* groupLogo = myUICanvas.GetImage("GrandmaInvasionLogo");
//    mySplashScreens.push_back(groupLogo);
//	mySplashScreensShownTime.push_back(1.5f);
//    mySplashScreens.push_back(myUICanvas.GetImage("FMODLogo"));
//	mySplashScreensShownTime.push_back(0.5f);
//
//	mySplashScreenAnimator = Animator2D(&groupLogo->instance, &groupLogo->shared, 2, 512, {Animation2D("Idle", { 1,2 }, true)});
//	mySplashScreenAnimator.PlayAnimation2D("Idle");
//
//	for (int i = 1; i < mySplashScreens.size(); i++)
//	{
//		mySplashScreens[i]->tint = Tga::Color(0.f, 0.f, 0.f, 0.f);
//	}
//	mySplashFadeTime = 0.75f;
//}
//
//void SplashScreenBehaviour::Update()
//{
//	mySplashScreenAnimator.Update();
//	mySplashScreenCurrentTime += Essentials::GetDeltaTime();
//
//	if (mySplashScreenCurrentTime < mySplashFadeTime)
//	{
//		float colorValue = std::lerp(0.f, 1.f, mySplashScreenCurrentTime / mySplashFadeTime);
//		mySplashScreens[mySplashScreenIndex]->tint = Tga::Color(colorValue, colorValue, colorValue, colorValue);
//	}
//	else if (mySplashScreenCurrentTime > mySplashFadeTime * 2 + mySplashScreensShownTime[mySplashScreenIndex])
//	{
//		mySplashScreenIndex++;
//		if (mySplashScreenIndex < mySplashScreens.size())
//		{
//			mySplashScreenCurrentTime = 0.f;
//			mySplashScreens[mySplashScreenIndex]->tint = Tga::Color(0.f, 0.f, 0.f, 0.f);
//			return;
//		}
//
//		Essentials::globalSceneManager->RequestScene("TitleScene.tgs");
//		return;
//	}
//	else if (mySplashScreenCurrentTime > mySplashFadeTime + mySplashScreensShownTime[mySplashScreenIndex])
//	{
//		float colorValue = std::lerp(1.f, 0.f, (mySplashScreenCurrentTime - (mySplashFadeTime + mySplashScreensShownTime[mySplashScreenIndex])) / mySplashFadeTime);
//		mySplashScreens[mySplashScreenIndex]->tint = Tga::Color(colorValue, colorValue, colorValue, colorValue);
//	}
//}
//
//void SplashScreenBehaviour::Render()
//{
//	mySplashScreenAnimator.Render();
//}
