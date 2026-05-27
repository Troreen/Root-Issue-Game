#include "MainMenuComponent.h"
#include "Essentials.h"
#include "WorldTransitionService.h"
#include "StateStack.hpp"

void MainMenuComponent::Init(Tga::Engine& /*anEngine*/)
{
	Tga::Engine::GetInstance()->SetClearColor(Tga::Color(0, 0, 0, 1));
	myUICanvas.Init("MainMenu", *Essentials::globalCanvasManager);
	myUICanvas.SetSelectedElement("StartButton");
	if (UIToggle* fullscreenToggle = myUICanvas.GetToggle("FullscreenToggle"))
	{
		fullscreenToggle->isOn = Tga::Engine::GetInstance()->GetIsBorderless();
	}

	if (UISlider* masterVolumeSlider = myUICanvas.GetSlider("MasterVolumeSlider"))
	{
		masterVolumeSlider->currentValue = Essentials::globalAudioManager->GetMasterVolume();
	}
	if (UISlider* musicVolumeSlider = myUICanvas.GetSlider("MusicVolumeSlider"))
	{
		musicVolumeSlider->currentValue = Essentials::globalAudioManager->GetBusVolume(BusID::eMusic);
	}
	if (UISlider* sfxVolumeSlider = myUICanvas.GetSlider("SfxVolumeSlider"))
	{
		sfxVolumeSlider->currentValue = Essentials::globalAudioManager->GetBusVolume(BusID::eSFX);
	}

	myUICanvas.SetIsHidden("SettingsMenu", true);
	//myUICanvas.SetIsHidden("LevelSelectMenu", true);
	myUICanvas.SetIsHidden("MainMenu", false);

}

void MainMenuComponent::OnUpdate(float /*aDeltaTime*/)
{
	if (Essentials::globalSceneManager->HasRequestedScene())
		return;

	/*if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eMainMenu))
	{
		Essentials::globalAudioManager->PlayMusic(SoundID::eMainMenu);
	}*/

	if (myUICanvas.GetElementPressed("SettingsButton"))
	{
		myUICanvas.SetIsHidden("SettingsMenu", false);
		myUICanvas.SetIsHidden("MainMenu", true);
		myUICanvas.SetSelectedElement("BackButtonSettings");
	}
	else if (myUICanvas.GetElementPressed("BackButtonSettings") || (Essentials::globalInputManager->IsButtonPressed(Tga::GamepadButton::B) && !myUICanvas.GetIsHidden("SettingsMenu")))
	{
		myUICanvas.SetIsHidden("SettingsMenu", true);
		myUICanvas.SetIsHidden("MainMenu", false);
		myUICanvas.SetSelectedElement("SettingsButton");

		//Essentials::SaveSettings();
	}
	//else if (myUICanvas.GetElementPressed("LevelSelectButton"))
	//{
	//	myUICanvas.SetIsHidden("LevelSelectMenu", false);
	//	myUICanvas.SetIsHidden("MainMenu", true);
	//	myUICanvas.SetSelectedElement("BackButtonLevelSelect");
	//}
	//else if (myUICanvas.GetElementPressed("BackButtonLevelSelect") || (Essentials::globalInputManager->IsButtonPressed(Tga::GamepadButton::B) && !myUICanvas.GetIsHidden("LevelSelectMenu")))
	//{
	//	myUICanvas.SetIsHidden("LevelSelectMenu", true);
	//	myUICanvas.SetIsHidden("MainMenu", false);
	//	myUICanvas.SetSelectedElement("LevelSelectButton");
	//}
	//else if (myUICanvas.GetElementPressed("CreditsButton"))
	//{
	//	myUICanvas.SetIsHidden("CreditsMenu", false);
	//	myUICanvas.SetIsHidden("MainMenu", true);
	//	myUICanvas.SetSelectedElement("BackButtonCredits");
	//}
	//else if (myUICanvas.GetElementPressed("BackButtonCredits") || (Essentials::globalInputManager->IsButtonPressed(Tga::GamepadButton::B) && !myUICanvas.GetIsHidden("CreditsMenu")))
	//{
	//	myUICanvas.SetIsHidden("CreditsMenu", true);
	//	myUICanvas.SetIsHidden("MainMenu", false);
	//	myUICanvas.SetSelectedElement("CreditsButton");
	//}
	else if (myUICanvas.GetElementPressed("StartButton"))
	{
		myUICanvas.SetIsHidden(true);

		//Essentials::myCursor->SetCursorVisible(false);

		/*if (Essentials::globalAudioManager->IsEventPlaying(SoundID::eMainMenu))
		{
			Essentials::globalAudioManager->StopMusic(SoundID::eMainMenu, false);
			Essentials::globalAudioManager->PlayMusic(SoundID::eTransition);
		}*/

		//Essentials::globalSceneManager->RequestScene("IntroScene.tgs");

		
		//WorldTransitionService::RequestSceneTransition("Levels/TestScenes/PabloTestingScene.tgs", "", 0);

		//return; -> Me dont know why this is here help / Pablo
	}
	else if (myUICanvas.GetElementPressed("FullscreenToggle"))
	{
		Tga::Engine::GetInstance()->SetBorderless(myUICanvas.GetToggleValue("FullscreenToggle"));
	}
	else if (myUICanvas.GetSelectedElementName() == "MasterVolumeSlider")
	{
		Essentials::globalAudioManager->SetMasterVolume(myUICanvas.GetSliderValue("MasterVolumeSlider"));
	}
	else if (myUICanvas.GetSelectedElementName() == "MusicVolumeSlider")
	{
		Essentials::globalAudioManager->SetBusVolume(BusID::eMusic, myUICanvas.GetSliderValue("MusicVolumeSlider"));
	}
	else if (myUICanvas.GetSelectedElementName() == "SfxVolumeSlider")
	{
		Essentials::globalAudioManager->SetBusVolume(BusID::eSFX, myUICanvas.GetSliderValue("SfxVolumeSlider"));
	}
	else if (myUICanvas.GetElementPressed("QuitButton"))
	{
		Essentials::Shutdown();
	}
	//else
	//{
	//	int pressedLevel = 0;
	//	if (myUICanvas.GetElementPressed("Level1Button"))
	//		pressedLevel = 1;
	//	if (myUICanvas.GetElementPressed("Level2Button"))
	//		pressedLevel = 2;
	//	if (myUICanvas.GetElementPressed("Level3Button"))
	//		pressedLevel = 3;

	//	if (pressedLevel != 0)
	//	{
	//		Essentials::myCursor->SetCursorVisible(false);

	//		//if (Essentials::globalAudioManager->IsEventPlaying(SoundID::eMainMenu))
	//		//{
	//		//	Essentials::globalAudioManager->StopMusic(SoundID::eMainMenu, false);
	//		//}

	//		//Essentials::globalSceneManager->RequestScene("MainGame.tgs");

	//		return;
	//	}
	//}
}
