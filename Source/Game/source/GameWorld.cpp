#include "GameWorld.h"
#include "GameObject.h"

#include "GameObjectFactoryRegistrations.h"
#include "MeshComponent.h"
#include "SceneImportService.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
#include <iostream>
#include <limits>

#include <Windows.h>

#include <tge/engine.h>
#include <tge/animation/Script/AnimationNodes.h>
#include <tge/error/ErrorManager.h>
#include <tge/graphics/DX11.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/script/Nodes/CommonMathNodes.h>
#include <tge/script/Nodes/CommonNodes.h>

#include "PlayerControllerComponent.h"

#include "Essentials.h"

#include "StateStack.hpp"
#include "InGame.h"
#include "MainMenu.h"
#include "Options.h"
#include "SplashScreen.h"
#include <tge/settings/settings.h>


namespace
{
	void RegisterAnimationGraphNodesOnce()
	{
		static bool isRegistered = false;
		if (isRegistered)
		{
			return;
		}

		Tga::RegisterCommonNodes();
		Tga::RegisterCommonMathNodes();
		Tga::RegisterAnimationNodes();

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

	bool RestoreGameWindowFocusIfNeeded()
	{
		Tga::Engine* engine = Tga::Engine::GetInstance();
		if (!engine || !engine->GetHWND())
		{
			return false;
		}

		HWND hwnd = *engine->GetHWND();
		if (!hwnd)
		{
			return false;
		}

		const HWND foreground = GetForegroundWindow();

		if (foreground != hwnd)
		{
			const DWORD currentThread = GetCurrentThreadId();
			DWORD foregroundThread = 0;
			if (foreground)
			{
				foregroundThread = GetWindowThreadProcessId(foreground, nullptr);
			}


			if (foregroundThread != 0 && foregroundThread != currentThread)
			{
				AttachThreadInput(currentThread, foregroundThread, TRUE);
			}

			if (IsIconic(hwnd))
			{
				ShowWindow(hwnd, SW_RESTORE);
			}

			SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			SetForegroundWindow(hwnd);
			SetActiveWindow(hwnd);
			SetFocus(hwnd);

			if (foregroundThread != 0 && foregroundThread != currentThread)
			{
				AttachThreadInput(currentThread, foregroundThread, FALSE);
			}

		}
		else
		{
			SetActiveWindow(hwnd);
			SetFocus(hwnd);
		}

		const bool hasForeground = (GetForegroundWindow() == hwnd);
		const bool hasFocus = (GetFocus() == hwnd) || (GetActiveWindow() == hwnd);
		return hasForeground && hasFocus;
	}

}

using namespace Tga;

GameWorld::GameWorld()
	: myLoadingText("Text/Evil Bible.ttf", Tga::FontSize_72)
	, myCameraSystem(*Essentials::globalCamera.get())
{
	mySceneName.clear();
	mySceneLoadTarget.clear();
	myQueuedSceneRequest.clear();
	myIsSceneLoading = false;
	myPendingFocusRecoveryFrames = 0;
	myEnablePointLights = true;
	myEnableDirectionalLight = true;
	myEnableAmbientLight = true;
	myIsFirstFrame = true;
}

GameWorld::~GameWorld()
{
	if (mySceneLoadFuture.valid())
	{
		mySceneLoadFuture.wait();
	}

	VfxService::Set(nullptr);
}

void GameWorld::Init(const char* argv[])
{
	myWorldStateStack;
	myWorldStateStack.PushStack(std::vector<State*>());
	myWorldStateStack.GetCurrentStateStack()->push_back(new InGame());

	myWorldStateStack.GetCurrentState()->Init(myCameraSystem, argv);

	std::string rootPath = Tga::Settings::GameAssetRoot().string();
	Essentials::globalCanvasManager->Init(rootPath);

	Essentials::globalAudioManager->Init();
#ifdef _DEBUG
	RegisterCommands(argv);
#endif
}

void GameWorld::Update(float /* aDeltaTime */, const char* argv[])
{
	switch (myWorldStateStack.GetCurrentState()->Update())
	{
	case eState::eMainMenu:
		myWorldStateStack.GetCurrentStateStack()->push_back(new MainMenu());

		myWorldStateStack.GetCurrentState()->Init(myCameraSystem, argv);
		break;
	case eState::eOptions:
		myWorldStateStack.GetCurrentStateStack()->push_back(new Options());
		myWorldStateStack.GetCurrentState()->Init(myCameraSystem, argv);
		break;
	case eState::ePlaying:
		myWorldStateStack.GetCurrentStateStack()->push_back(new InGame());
		myWorldStateStack.GetCurrentState()->Init(myCameraSystem, argv);
		break;
	case eState::eSplashScreen:
		myWorldStateStack.GetCurrentStateStack()->push_back(new SplashScreen());
		myWorldStateStack.GetCurrentState()->Init(myCameraSystem, argv);
		break;
	case eState::ePopState:
		myWorldStateStack.PopState();
		myWorldStateStack.GetCurrentStateStack()->back()->SetCamera(myCameraSystem);
		break;
	case eState::ePopStack:
		myWorldStateStack.PopStack();
		break;
	default:
		return;
	}

	//if (myInputHandler.IsKeyPressed(Keys::F3))
	//{
	//	DumpSceneVisibilitySnapshot(myGameObjects, myCameraSystem);
	//}

	//myCameraSystem.UpdateDebugCamera(deltaTime, myInputHandler);
	//myCameraSystem.Update(deltaTime);
	//

	//for (auto& object : myGameObjects)
	//{
	//	if (!object || !object->IsActive())
	//	{
	//		continue;
	//	}
	//	object->Update(deltaTime);
	//}

	//myVfxSystem.Update(deltaTime);
}

void GameWorld::ClearSceneObjects()
{
	for (auto it = myGameObjects.begin(); it != myGameObjects.end();)
	{
		if (!(*it) || !(*it)->IsPersistent())
		{
			it = myGameObjects.erase(it);
			continue;
		}

		++it;
	}
}

void GameWorld::LoadScene(const std::string& aScenePath)
{
	SceneImportService importer;
	auto importedObjects = importer.BuildGameObjects(aScenePath);
	ApplyLoadedScene(std::move(importedObjects), aScenePath);
}

void GameWorld::StartSceneLoadAsync(const std::string& aScenePath, const bool aForceReload)
{
	if (aScenePath.empty() || (!aForceReload && aScenePath == mySceneName))
	{
		return;
	}

	// Async loading is temporarily disabled. Keep the old implementation for easy rollback.
#if 0
	myVfxSystem.BeginSceneTransition(mySceneName, aScenePath);

	mySceneLoadTarget = aScenePath;

	myIsSceneLoading = true;
	mySceneLoadFuture = std::async(std::launch::async, [scenePath = aScenePath]()
		{
			SceneImportService importer;
			return importer.LoadSceneObjects(scenePath);
		});
#endif

	myVfxSystem.BeginSceneTransition(mySceneName, aScenePath);
	mySceneLoadTarget = aScenePath;
	myIsSceneLoading = false;
	LoadScene(aScenePath);
	mySceneLoadTarget.clear();

	myPendingFocusRecoveryFrames = 120;
	TryRecoverWindowFocus();
	if (Tga::Engine* engine = Tga::Engine::GetInstance(); engine && engine->GetHWND())
	{
		myInputHandler.SetWindowHandle(*engine->GetHWND());
	}
}

void GameWorld::PollSceneLoadCompletion()
{
	// Async loading is temporarily disabled. Keep the old implementation for easy rollback.
#if 0
	if (!myIsSceneLoading || !mySceneLoadFuture.valid())
	{
		return;
	}

	if (mySceneLoadFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
	{
		return;
	}

	auto sceneObjects = mySceneLoadFuture.get();
	const std::string loadedScene = mySceneLoadTarget;
	mySceneLoadTarget.clear();
	myIsSceneLoading = false;
	auto importedObjects = BuildSceneObjects(sceneObjects);
	ApplyLoadedScene(std::move(importedObjects), loadedScene);

	myPendingFocusRecoveryFrames = 120;
	TryRecoverWindowFocus();
	if (Tga::Engine* engine = Tga::Engine::GetInstance(); engine && engine->GetHWND())
	{
		myInputHandler.SetWindowHandle(*engine->GetHWND());
	}

	if (!myQueuedSceneRequest.empty() && myQueuedSceneRequest != mySceneName)
	{
		const std::string queuedScene = myQueuedSceneRequest;
		myQueuedSceneRequest.clear();
		StartSceneLoadAsync(queuedScene);
	}
#endif
}

void GameWorld::UnloadActiveLevel(const bool aClearSceneName)
{
	myVfxSystem.ClearActiveEffects();

	myCameraSystem.ResetTransientEffects();
	myCameraSystem.SetSceneName("");

	if (aClearSceneName)
	{
		mySceneName.clear();
		Essentials::globalSceneManager->SetCurrentScene("");
	}

	ClearSceneObjects();
}

void GameWorld::RegisterCommands(const char* argv[])
{
	auto Console = Essentials::GetEssentials().globalConsoleManager.get();
	Console->Toggle();
	Console->InitCommands();

	Console->RegisterCommand("Debug", "LS", "load", "Load state. Example: LS 1, LS 2",
		[this, argv](const std::vector<std::string>& args)
		{
			switch (static_cast<int>(args[0][0]))
			{
			case 49:
				myWorldStateStack.GetCurrentStateStack()->push_back(new InGame());
				myWorldStateStack.GetCurrentState()->Init(myCameraSystem, argv);
				break;
				// TEMP STATES!
			case 50:
				myWorldStateStack.GetCurrentStateStack()->push_back(new Options());
				myWorldStateStack.GetCurrentState()->Init(myCameraSystem, argv);
				break;
			case 51:
				myWorldStateStack.GetCurrentStateStack()->push_back(new MainMenu());
				myWorldStateStack.GetCurrentState()->Init(myCameraSystem, argv);
				break;
			case 52:
				myWorldStateStack.GetCurrentStateStack()->push_back(new SplashScreen());
				myWorldStateStack.GetCurrentState()->Init(myCameraSystem, argv);
				// END TEMP STATES
			default:
				break;
			}
		}
	);

	Console->RegisterCommand("Debug", "PS", "pop", "Pop current state.",
		[this](const std::vector<std::string>& /*args*/)
		{
			myWorldStateStack.PopState();
		}
	);

	//Console->RegisterCommand("Debug", "GM", "GODMODE", "Enter god mode.",
	//	[this](const std::vector<std::string>& /*args*/)
	//	{
	//		auto GetState = myWorldStateStack.GetCurrentState()->GetPlayer()->GetComponent<PlayerControllerComponent>()->GetState();


	//		if (GetState->GetSpeed() > 1000.0f)
	//		{
	//			GetState->SetSpeed(600.0f);
	//		}
	//		else
	//		{
	//			GetState->SetSpeed(1800.0f);
	//		}
	//	}
	/*);*/
	Console->RegisterCommand("Debug", "LPS", "Load scene", "Load playable scene. Use name of the scene.",
		[this, argv](const std::vector<std::string>& args)
		{
			std::string name;
			for (int i = 0; i < args[0].size(); i++)
			{
				name = name + args[0][i];
			}
			std::cout << name << std::endl;

			std::string fullname = "Levels/" + name + ".tgs";
			myWorldStateStack.GetCurrentStateStack()->push_back(new InGame());
			argv[1] = fullname.c_str();
			myWorldStateStack.GetCurrentState()->Init(myCameraSystem, argv);

			return;
		}

	);
	Console->ListCommands();
}

void GameWorld::TryRecoverWindowFocus()
{
	if (myPendingFocusRecoveryFrames <= 0)
	{
		return;
	}

	RestoreGameWindowFocusIfNeeded();
	--myPendingFocusRecoveryFrames;
}

void GameWorld::ApplyLoadedScene(std::vector<std::unique_ptr<GameObject>>&& someObjects, const std::string& aScenePath)
{
	mySceneName = aScenePath;
	myCameraSystem.SetSceneName(mySceneName);
	myCameraSystem.ResetTransientEffects();
	Essentials::globalSceneManager->SetCurrentScene(mySceneName);

	ClearSceneObjects();

	Tga::Engine* engine = Tga::Engine::GetInstance();
	if (!engine)
	{
		ERROR_PRINT("GameWorld::ApplyLoadedScene failed to access engine instance.");
		return;
	}

	std::cout << "[GameWorld] Loaded " << someObjects.size() << " objects from scene: " << mySceneName << "\n";

	for (auto& object : someObjects)
	{
		if (!object)
		{
			continue;
		}

		object->Init(*engine);
		myGameObjects.push_back(std::move(object));
	}

}

void GameWorld::Render()
{
#ifdef _DEBUG
	auto Console = Essentials::GetEssentials().globalConsoleManager.get();
	Console->Draw();
#endif


	myWorldStateStack.GetCurrentState()->Render();

	if (!myIsFirstFrame)
	{
		UICanvas::RenderAll();
	}

	if (myIsFirstFrame)
	{
		myIsFirstFrame = false;
	}

}

void GameWorld::RenderLoadingScreen()
{
	Tga::Engine* engine = Tga::Engine::GetInstance();
	if (!engine)
	{
		return;
	}

	auto& graphicsEngine = engine->GetGraphicsEngine();
	auto& graphicsStateStack = graphicsEngine.GetGraphicsStateStack();

	const int dotCount = static_cast<int>(std::fmod(myTimer.GetTotalTime() * 2.0, 4.0));
	std::string loadingText = "Loading";
	loadingText.append(static_cast<size_t>(dotCount), '.');

	const Tga::Vector2ui resolution = engine->GetRenderSize();
	myLoadingText.SetText(loadingText);
	myLoadingText.SetPosition({
		0.5f * static_cast<float>(resolution.x) - 0.5f * myLoadingText.GetWidth(),
		0.5f * static_cast<float>(resolution.y)
		});

	Tga::DX11::BackBuffer->SetAsActiveTarget();
	Tga::DX11::BackBuffer->Clear({ 0.0f, 0.0f, 0.0f, 1.0f });
	graphicsStateStack.SetDefaultCamera();
	myLoadingText.Render();
}

void GameWorld::RenderDefault()
{
	mySceneRenderer.Render(
		myGameObjects,
		myCameraSystem,
		myVfxSystem,
		myEnablePointLights,
		myEnableDirectionalLight,
		myEnableAmbientLight,
		gEnableFrustumCulling);

#ifdef _DEBUG
	myCameraSystem.RenderDebugUi();
#endif
}

CommonUtilities::InputHandler& GameWorld::GetInputHandler()
{
	return myInputHandler;
}

CommonUtilities::Timer& GameWorld::GetTimer()
{
	return myTimer;
}

CommonUtilities::Camera3Df* GameWorld::GetCamera()
{
	return &myCameraSystem.GetCamera();
}

float GameWorld::GetDeltaTime() const
{
	return myTimer.GetDeltaTime();
}
