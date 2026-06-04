#include "PauseMenuComponent.h"
#include "Essentials.h"


PauseMenuComponent::PauseMenuComponent() = default;

void PauseMenuComponent::Init(Tga::Engine& /*anEngine*/)
{
	myUICanvas.Init("PauseMenu", *Essentials::globalCanvasManager);
	myUICanvas.SetSelectedElement("ResumeButton");
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
	myUICanvas.SetIsHidden(true);
	myUICanvas.SetIsHidden("PauseMenu", false);
	myIsOpen = false;
	myReturnToMainMenu = false;
	Essentials::InitCursor();
	Essentials::myCursor->SceneLoaded();
	Essentials::globalEngine->LockCursorToWindow();
}

void PauseMenuComponent::OnUpdate(float /*aDeltaTime*/)
{
	if (myUICanvas.GetIsHidden())
	{
		if (Essentials::globalInputManager->IsKeyPressed(static_cast<int>(Keys::ESCAPE)) || Essentials::globalInputManager->IsButtonPressed(Tga::GamepadButton::Start))
		{
			myUICanvas.SetIsHidden(false);
			//myIsActive = true;
			myIsOpen = true;
			Essentials::globalEngine->UnlockCursor();
			Essentials::globalEngine->SetTimeScale(0.f);
			//if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::ePauseMenu))
			//{
			//	Essentials::globalAudioManager->SetEventVolume(eLevelOne, 0);
			//	Essentials::globalAudioManager->SetEventVolume(eWind, 0);
			//	Essentials::globalAudioManager->SetEventVolume(eRain, 0);
			//	Essentials::globalAudioManager->SetEventVolume(eBirb, 0);
			//	Essentials::globalAudioManager->SetEventVolume(eBootsKnight, 0);
			//	Essentials::globalAudioManager->SetEventVolume(eLevelTwo, 0);
			//	Essentials::globalAudioManager->SetEventVolume(eLevelThree, 0);
			//	Essentials::globalAudioManager->SetEventVolume(eLevelFour, 0);
			//	Essentials::globalAudioManager->PlayMusic(SoundID::ePauseMenu);
			//}
		}
		return;
	}
	else
	{
		if (Essentials::globalInputManager->IsKeyPressed(static_cast<int>(Keys::ESCAPE)) ||
			Essentials::globalInputManager->IsButtonPressed(Tga::GamepadButton::Start) ||
			myUICanvas.GetElementPressed("ResumeButton"))
		{
			Essentials::myCursor->SetCursorVisible(false);
			myUICanvas.SetIsHidden(true);
			//myIsActive = false;
			myIsOpen = false;
			//Essentials::globalAudioManager->SetEventVolume(eLevelOne, 1);
			//Essentials::globalAudioManager->SetEventVolume(eWind, 1);
			//Essentials::globalAudioManager->SetEventVolume(eRain, 1);
			//Essentials::globalAudioManager->SetEventVolume(eBirb, 1);
			//Essentials::globalAudioManager->SetEventVolume(eBootsKnight, 1);
			//Essentials::globalAudioManager->SetEventVolume(eLevelTwo, 1);
			//Essentials::globalAudioManager->SetEventVolume(eLevelThree, 1);
			//Essentials::globalAudioManager->SetEventVolume(eLevelFour, 1);
			//Essentials::globalAudioManager->StopMusic(SoundID::ePauseMenu, true);

			if (!myUICanvas.GetIsHidden("SettingsPauseMenu"))
			{
				myUICanvas.SetIsHidden("SettingsPauseMenu", true);
				myUICanvas.SetIsHidden("PauseMenu", false);
			}

			myUICanvas.SetSelectedElement("ResumeButton");
			Essentials::globalEngine->SetTimeScale(1.f);
			Essentials::globalEngine->LockCursorToWindow();
			myUICanvas.ResetIsFocused();
		}
	}


	if (myUICanvas.GetElementPressed("SettingsButton"))
	{
		myUICanvas.SetIsHidden("SettingsPauseMenu", false);
		myUICanvas.SetIsHidden("PauseMenu", true);
		myUICanvas.SetSelectedElement("BackButtonSettings");
	}
	else if (myUICanvas.GetElementPressed("BackButtonSettings") || (Essentials::globalInputManager->IsButtonPressed(Tga::GamepadButton::B) && !myUICanvas.GetIsHidden("SettingsMenu")))
	{
		myUICanvas.SetIsHidden("SettingsPauseMenu", true);
		myUICanvas.SetIsHidden("PauseMenu", false);
		myUICanvas.SetSelectedElement("SettingsButton");

		//Essentials::SaveSettings();
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
	else if (myUICanvas.GetElementPressed("MainMenuButton"))
	{
		Essentials::myCursor->SetCursorVisible(false);
	//	//Essentials::globalAudioManager->SetEventVolume(eLevelOne, 1);
	//	//myLevelButtons[0] = myUICanvas.GetElement("Level1Button")->definition;
	//	//Essentials::globalAudioManager->SetEventVolume(eLevelTwo, 1);
	//	//Essentials::globalAudioManager->SetEventVolume(eLevelThree, 1);
	//	//Essentials::globalAudioManager->SetEventVolume(eLevelFour, 1);
	//	//Essentials::globalAudioManager->SetEventVolume(eWind, 1);
	//	//Essentials::globalAudioManager->SetEventVolume(eRain, 1);
	//	//Essentials::globalAudioManager->SetEventVolume(eBirb, 1);
	//	//Essentials::globalAudioManager->SetEventVolume(eBootsKnight, 1);
	//	//Essentials::globalAudioManager->StopMusic(SoundID::eLevelOne, true);
	//	//Essentials::globalAudioManager->StopMusic(SoundID::eLevelTwo, true);
	//	//Essentials::globalAudioManager->StopMusic(SoundID::eLevelThree, true);
	//	//Essentials::globalAudioManager->StopMusic(SoundID::eLevelFour, true);
	//	//Essentials::globalAudioManager->StopMusic(SoundID::eWind, true);
	//	//Essentials::globalAudioManager->StopMusic(SoundID::eRain, true);
	//	//Essentials::globalAudioManager->StopMusic(SoundID::eBirb, true);
	//	//Essentials::globalAudioManager->StopMusic(SoundID::eBootsKnight, true);
	//	//Essentials::globalAudioManager->StopMusic(SoundID::ePauseMenu, true);

	    Tga::Engine::GetInstance()->SetTimeScale(1.f);
		myReturnToMainMenu = true;
	    return;
	}
}

bool PauseMenuComponent::IsOpen()
{
	return myIsOpen;
}

bool PauseMenuComponent::ReturnToMainMenu() const
{
	return myReturnToMainMenu;
}
