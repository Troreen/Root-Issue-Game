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

#include "Essentials.h"

#include "StateStack.hpp"
#include "InGame.h"
#include "MainMenu.h"
#include "Options.h"

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

	Essentials::globalAudioManager->Init();
}

void GameWorld::Update(float deltaTime, const char* argv[])
{
	myWorldStateStack.GetCurrentState()->Update();
	if (Essentials::GetEssentials().globalInputManager->IsKeyReleased(static_cast<int>(Keys::P)))
	{
		myWorldStateStack.GetCurrentStateStack()->push_back(new InGame());

		myWorldStateStack.GetCurrentState()->Init(myCameraSystem, argv);
	}
	if (Essentials::GetEssentials().globalInputManager->IsKeyReleased(static_cast<int>(Keys::O)))
	{
		
		auto* camera = myWorldStateStack.GetCurrentStateStack()->back()->GetCameraSystem();
		myWorldStateStack.PopState();
		myWorldStateStack.GetCurrentStateStack()->back()->SetCamera(*camera);

	}

	if (myInputHandler.IsKeyPressed(Keys::F3))
	{
		DumpSceneVisibilitySnapshot(myGameObjects, myCameraSystem);
	}

	myCameraSystem.UpdateDebugCamera(deltaTime, myInputHandler);
	myCameraSystem.Update(deltaTime);

	for (auto& object : myGameObjects)
	{
		if (!object || !object->IsActive())
		{
			continue;
		}

		object->Update(deltaTime);
	}

	myVfxSystem.Update(deltaTime);
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
	myWorldStateStack.GetCurrentState()->Render();
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
