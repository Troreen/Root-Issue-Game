#pragma once
#include "UICanvas.h"
#include "Animator2D.h"
#include "ScriptComponent.h"

class SplashScreenComponent : public ScriptComponent
{
	public:
		SplashScreenComponent();
		void Init(Tga::Engine& anEngine) override;
		void OnUpdate(float aDeltaTime) override;
		void Render() override;

	private:

		UICanvas myUICanvas;
		Tga::StringId myCanvasName;
		float mySplashFadeTime;
		std::vector<UIImage*> mySplashScreens;
		std::vector<float> mySplashScreensShownTime;
		float mySplashScreenCurrentTime;
		int mySplashScreenIndex;
		Animator2D mySplashScreenAnimator;
};


