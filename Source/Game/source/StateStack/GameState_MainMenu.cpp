#include "MainMenu.h"
#include "AudioManager.h"
#include "Essentials.h"

#include <tge/animation/Script/AnimationNodes.h>
#include <tge/engine.h>
#include <tge/input/InputManager.h>
#include <tge/script/Nodes/CommonMathNodes.h>
#include <tge/script/Nodes/CommonNodes.h>

#include "MeshComponent.h"
#include "GameObject.h"

#include <algorithm>
#include <iostream>
#include <limits>

namespace
{
	void RegisterAnimationGraphNodesOnce()
	{
		static bool isRegistered = false;
		if (isRegistered)
		{
			return;
		}

		/*Tga::RegisterCommonNodes();
		Tga::RegisterCommonMathNodes();
		Tga::RegisterAnimationNodes();*/

		isRegistered = true;
	}

	bool gEnableFrustumCulling = true;

	void DumpSceneVisibilitySnapshot(
		const std::vector<std::unique_ptr<GameObject>>& someObjects,
		const CameraSystem& aCameraSystem)
	{
		const CommonUtilities::Camera3Df& camera = aCameraSystem.GetCamera();
		const auto cameraPosition = camera.GetTransform().GetPosition();
		const auto cameraForward = camera.GetTransform().GetForward();

		size_t activeObjectCount = 0;
		size_t meshComponentCount = 0;
		size_t validMeshComponentCount = 0;
		size_t meshDefaultCount = 0;
		size_t meshLambertCount = 0;
		size_t meshPbrCount = 0;
		size_t meshCustomCount = 0;

		float nearestDistance = (std::numeric_limits<float>::max)();
		float farthestDistance = 0.0f;

		for (const std::unique_ptr<GameObject>& object : someObjects)
		{
			if (!object || !object->IsActive())
			{
				continue;
			}

			++activeObjectCount;
			const float distanceToCamera = (object->GetTransform().GetPosition() - cameraPosition).Length();
			nearestDistance = (std::min)(nearestDistance, distanceToCamera);
			farthestDistance = (std::max)(farthestDistance, distanceToCamera);

			if (MeshComponent* mesh = object->GetComponent<MeshComponent>())
			{
				++meshComponentCount;
				if (mesh->IsValid())
				{
					++validMeshComponentCount;

					switch (mesh->GetRenderMode())
					{
					case MeshComponent::RenderMode::Lambert:
						++meshLambertCount;
						break;
					case MeshComponent::RenderMode::Pbr:
						++meshPbrCount;
						break;
					case MeshComponent::RenderMode::Custom:
						++meshCustomCount;
						break;
					case MeshComponent::RenderMode::Default:
					default:
						++meshDefaultCount;
						break;
					}
				}
			}
		}

		if (activeObjectCount == 0)
		{
			nearestDistance = 0.0f;
		}

		std::cout << "[RenderDebug] cameraPos=(" << cameraPosition.x << ", " << cameraPosition.y << ", " << cameraPosition.z
			<< ") cameraForward=(" << cameraForward.x << ", " << cameraForward.y << ", " << cameraForward.z << ")"
			<< " near=" << camera.GetNearPlane()
			<< " far=" << camera.GetFarPlane() << "\n";

		std::cout << "[RenderDebug] activeObjects=" << activeObjectCount
			<< " meshComponents=" << meshComponentCount
			<< " validMeshes=" << validMeshComponentCount
			<< " renderModes(default/lambert/pbr/custom)="
			<< meshDefaultCount << "/" << meshLambertCount << "/" << meshPbrCount << "/" << meshCustomCount
			<< " nearestObjectDist=" << nearestDistance
			<< " farthestObjectDist=" << farthestDistance
			<< " frustumCulling=" << (gEnableFrustumCulling ? "ON" : "OFF") << "\n";

		if (activeObjectCount > 0 && nearestDistance < camera.GetNearPlane())
		{
			std::cout << "[RenderDebug] WARNING: nearest object is in front of near plane and may be clipped."
				<< " Lower near plane or move camera back.\n";
		}
	}
}


void MainMenu::Init(CameraSystem& aCamera, const char* argv[])
{
	Tga::Engine& engine = *Tga::Engine::GetInstance();
	myLoadingText = { "Text/Evil Bible.ttf", Tga::FontSize_72 };
	myCameraSystem = &aCamera;
	{
		mySceneName.clear();
		myPendingFocusRecoveryFrames = 0;
		myEnablePointLights = true;
		myEnableDirectionalLight = true;
		myEnableAmbientLight = true;
	}

	engine;
	argv;

	myCameraSystem->Init();

	mySceneName.clear();
	myCameraSystem->SetSceneName(mySceneName);

	myVfxSystem.Init();
	VfxService::Set(&myVfxSystem);

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
	myUICanvas.SetIsHidden("LevelSelectMenu", true);
	myUICanvas.SetIsHidden("CreditsMenu", true);
	myUICanvas.SetIsHidden("MainMenu", false);
	myUICanvas.SetIsHidden(false);
	myStartGame = false;
	myStartFirstLevel = false;
	myStartSecondLevel = false;
	myStartThirdLevel = false;
}

eState MainMenu::Update()
{
	/*myTimer.Update();
	myInputHandler.UpdateInput();*/
	const float deltaTime = Essentials::GetDeltaTime();


	myVfxSystem.Update(deltaTime);

	UpdateMainMenuUI();

	/*if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eMusicLoop))
	{
		Essentials::globalAudioManager->PlayMusic(SoundID::eMusicLoop);
	}*/
	Essentials::globalAudioManager->Update(deltaTime);

	UICanvas::UpdateAll();

	if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eMainMenuMusic))
	{
		Essentials::globalAudioManager->PlayMusic(SoundID::eMainMenuMusic);
	}

	if (myStartGame)
	{
		Essentials::globalAudioManager->StopMusic(SoundID::eMainMenuMusic, false);
		Essentials::globalAudioManager->StopAllEvents();
		return eState::eLoadInGameWithIntro;
	}
	else if (myStartFirstLevel)
	{
		Essentials::globalAudioManager->StopMusic(SoundID::eMainMenuMusic, false);
		Essentials::globalAudioManager->StopAllEvents();
		return eState::eLoadFirstLevel;
	}
	else if (myStartSecondLevel)
	{
		Essentials::globalAudioManager->StopMusic(SoundID::eMainMenuMusic, false);
		Essentials::globalAudioManager->StopAllEvents();
		return eState::eLoadSecondLevel;
	}
	else if (myStartThirdLevel)
	{
		Essentials::globalAudioManager->StopMusic(SoundID::eMainMenuMusic, false);
		Essentials::globalAudioManager->StopAllEvents();
		return eState::eLoadThirdLevel;
	}

	return eState::COUNT;
}

void MainMenu::Render()
{
	
	/*if (myIsSceneLoading)
	{
		RenderLoadingScreen();
		return;
	}
	Tga::Engine& engine = *Tga::Engine::GetInstance();
	auto& graphicsEngine = engine.GetGraphicsEngine();
	auto& graphicsStateStack = graphicsEngine.GetGraphicsStateStack();

	const Tga::Vector2ui resolution = engine.GetRenderSize();
	Tga::Text text("Text/arial.ttf", Tga::FontSize_72);
	text.SetText("You are in MainMenu!");
	text.SetPosition({
	0.5f * static_cast<float>(resolution.x) - 0.5f * text.GetWidth(),
	0.5f * static_cast<float>(resolution.y)
		});
	graphicsStateStack.SetDefaultCamera();

	text.Render();
	RenderDefault();*/
}

void MainMenu::UpdateMainMenuUI()
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
	else if (myUICanvas.GetElementPressed("LevelSelectButton"))
	{
		myUICanvas.SetIsHidden("LevelSelectMenu", false);
		myUICanvas.SetIsHidden("MainMenu", true);
		myUICanvas.SetSelectedElement("BackButtonLevelSelect");
	}
	else if (myUICanvas.GetElementPressed("BackButtonLevelSelect") || (Essentials::globalInputManager->IsButtonPressed(Tga::GamepadButton::B) && !myUICanvas.GetIsHidden("LevelSelectMenu")))
	{
		myUICanvas.SetIsHidden("LevelSelectMenu", true);
		myUICanvas.SetIsHidden("MainMenu", false);
		myUICanvas.SetSelectedElement("LevelSelectButton");
	}
	else if (myUICanvas.GetElementPressed("CreditsButton"))
	{
		myUICanvas.SetIsHidden("CreditsMenu", false);
		myUICanvas.SetIsHidden("MainMenu", true);
		myUICanvas.SetSelectedElement("BackButtonCredits");
	}
	else if (myUICanvas.GetElementPressed("BackButtonCredits") || (Essentials::globalInputManager->IsButtonPressed(Tga::GamepadButton::B) && !myUICanvas.GetIsHidden("CreditsMenu")))
	{
		myUICanvas.SetIsHidden("CreditsMenu", true);
		myUICanvas.SetIsHidden("MainMenu", false);
		myUICanvas.SetSelectedElement("CreditsButton");
	}
	else if (myUICanvas.GetElementPressed("StartButton"))
	{
		myUICanvas.SetIsHidden("MainMenu", false);
		myUICanvas.SetIsHidden("SettingsMenu", true);
		myUICanvas.SetIsHidden(true);

		myStartGame = true;

		//Essentials::myCursor->SetCursorVisible(false);

		/*if (Essentials::globalAudioManager->IsEventPlaying(SoundID::eMainMenu))
		{
			Essentials::globalAudioManager->StopMusic(SoundID::eMainMenu, false);
			Essentials::globalAudioManager->PlayMusic(SoundID::eTransition);
		}
		*/
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

	// Make this return a new eState, corresponding to desired new level.
	// Make new estate in the estate enum
	// Handle new estate in gameworld.cpp

	else
	{
		int pressedLevel = 0;
		if (myUICanvas.GetElementPressed("Level1Button"))
		{
			pressedLevel = 1;
			myStartFirstLevel = true;
		}
		if (myUICanvas.GetElementPressed("Level2Button"))
		{
			pressedLevel = 2;
			myStartSecondLevel = true;
		}
		if (myUICanvas.GetElementPressed("Level3Button"))
		{
			pressedLevel = 3;
			myStartThirdLevel = true;
		}

		if (pressedLevel != 0)
		{

			myUICanvas.SetIsHidden("MainMenu", false);
			myUICanvas.SetIsHidden("SettingsMenu", true);
			myUICanvas.SetIsHidden(true);

			//Essentials::myCursor->SetCursorVisible(false);

			//if (Essentials::globalAudioManager->IsEventPlaying(SoundID::eMainMenu))
			//{
			//	Essentials::globalAudioManager->StopMusic(SoundID::eMainMenu, false);
			//}

			//Essentials::globalSceneManager->RequestScene("MainGame.tgs");

			return;
		}
	}
}
