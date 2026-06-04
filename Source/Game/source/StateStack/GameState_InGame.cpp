#include "InGame.h"
#include "AudioManager.h"
#include <tge/animation/Script/AnimationNodes.h>
#include <tge/engine.h>
#include <tge/script/Nodes/AnimationEventNodes.h>
#include <tge/script/Nodes/CommonMathNodes.h>
#include <tge/script/Nodes/CommonNodes.h>

#include "BoxColliderComponent.h"
#include "CapsuleColliderComponent.h"
#include "CombatSystem.h"
#include "Essentials.h"
#include "GameObjectFactoryRegistrations.h"
#include "MeshComponent.h"
#include "GameObject.h"
#include "ObbColliderComponent.h"
#include "SphereColliderComponent.h"
#include "PauseMenuComponent.h"
#include "StaticSpriteComponent.h"

#include <tge/drawers/SpriteDrawer.h>
#include <tge/graphics/DX11.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/settings/Settings.h>
#include <tge/sprite/sprite.h>
#include <tge/texture/TextureManager.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <utility>

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
		Tga::RegisterAnimationEventNodes();

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


InGame::~InGame()
{
	if (Essentials::globalAnimationEvents)
	{
		Essentials::globalAnimationEvents->Clear();
	}
	RuntimeCollisionService::Clear();
	mySceneTransitionController.Shutdown();
	WorldTransitionService::SetListener(nullptr);
	WorldTransitionService::EndSequence();
}

void InGame::Init(CameraSystem& aCamera, const char* argv[])
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

	RegisterAnimationGraphNodesOnce();
	myCameraSystem->Init();
	CombatService::Set(&myCombatSystem);
	RuntimeCollisionService::Set(&myRuntimeCollisionSystem, &myGameObjects);

	RegisterGameObjectFactories();

	engine;

    /*myInputHandler.SetWindowHandle(*engine.GetHWND());
	myInputHandler.SetAutoMouseCapture(true);*/

	mySceneName.clear();
	myCameraSystem->SetSceneName(mySceneName);

	myVfxSystem.Init();
	VfxService::Set(&myVfxSystem);
	WorldTransitionService::SetListener(this);
	WorldTransitionService::EndSequence();
	mySceneTransitionController.Initialize(
		[this](std::vector<std::unique_ptr<GameObject>>&& someObjects, const std::string& aScenePath, const std::string& aTargetSpawnId)
		{
			ApplyTransitionScene(std::move(someObjects), aScenePath, aTargetSpawnId);
		},
		[this](const std::string& aFromScene, const std::string& aToScene)
		{
			myVfxSystem.BeginSceneTransition(aFromScene, aToScene);
		},
		[this]()
		{
			return mySceneName;
		});

	/*SceneManager& sceneManager = *Essentials::globalSceneManager;
	sceneManager.RefreshSceneList();*/

	if (argv[1] == nullptr)
	{
		mySceneTransitionController.LoadBootScene("Levels/HUB_00.tgs");
	}
	else
	{
		mySceneTransitionController.LoadBootScene(argv[1]);
	}
}

eState InGame::Update()
{
	CombatService::Set(&myCombatSystem);
	RuntimeCollisionService::Set(&myRuntimeCollisionSystem, &myGameObjects);

	myTimer.Update();
	myInputHandler.UpdateInput();

	TryRecoverWindowFocus();
	const float deltaTime = Essentials::GetDeltaTime();

	//PollSceneLoadCompletion();

	SceneManager& sceneManager = *Essentials::globalSceneManager;
	if (sceneManager.HasRequestedScene())
	{
		const std::string requested = sceneManager.ConsumeRequestedScene();

		if (!requested.empty() && requested != mySceneName)
		{
			SceneTransitionController::Request request;
			request.targetScene = requested;
			request.fadeSeconds = 0.5f;
			request.source = "SceneManager";
			mySceneTransitionController.RequestTransition(request);
		}
	}

	if (myInputHandler.IsKeyPressed(Keys::F2))
	{
		gEnableFrustumCulling = !gEnableFrustumCulling;
		std::cout << "[RenderDebug] Frustum culling " << (gEnableFrustumCulling ? "enabled" : "disabled") << "\n";
	}

	if (myInputHandler.IsKeyPressed(Keys::F3))
	{
		DumpSceneVisibilitySnapshot(myGameObjects, *myCameraSystem);
	}

	/*myCameraSystem->UpdateDebugCamera(deltaTime, myInputHandler);*/
	/*myCameraSystem->Update(deltaTime);*/

	UICanvas::UpdateAll();
	Essentials::myCursor->UpdatePosition();

	for (auto& object : myGameObjects)
	{
		if (!object || !object->IsActive())
		{
			continue;
		}

		object->Update(deltaTime);
		if (object->GetName() == "Player")
		{
			myPlayer = object.get();
		}
	}

	// Run after all object Updates so systems can react to frame-complete data,
	// such as animation events queued while animation graphs evaluated above.
	for (auto& object : myGameObjects)
	{
		if (!object || !object->IsActive())
		{
			continue;
		}

		object->LateUpdate(deltaTime);
	}

	if (Essentials::globalAnimationEvents)
	{
		Essentials::globalAnimationEvents->Update(deltaTime);
	}

	myRuntimeCollisionSystem.Run(myGameObjects);
	ConsumeCollisionContacts(myRuntimeCollisionSystem.GetContacts());
	myCombatSystem.Update(deltaTime, myGameObjects);

	myVfxSystem.Update(deltaTime);

	if (mySceneName == "Levels/Level1.tgs")
	{
		if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eMusicLevel1))
		{
			Essentials::globalAudioManager->StopMusic(SoundID::eMusicLevel2, true);
			Essentials::globalAudioManager->StopMusic(SoundID::eMusicLevel3, true);
			Essentials::globalAudioManager->StopMusic(SoundID::eMusicLoop, true);
			Essentials::globalAudioManager->PlayMusic(SoundID::eMusicLevel1);
		}
	}
	else if (mySceneName == "Levels/Level2.tgs")
	{
		if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eMusicLevel2))
		{
			Essentials::globalAudioManager->StopMusic(SoundID::eMusicLevel1, true);
			Essentials::globalAudioManager->StopMusic(SoundID::eMusicLevel3, true);
			Essentials::globalAudioManager->StopMusic(SoundID::eMusicLoop, true);
			Essentials::globalAudioManager->PlayMusic(SoundID::eMusicLevel2);
		}
	}
	else if (mySceneName == "Levels/Level3_Oskar.tgs" || mySceneName == "Levels/Level3_Part2_Oskar.tgs")
	{
		if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eMusicLevel3))
		{
			Essentials::globalAudioManager->StopMusic(SoundID::eMusicLevel1, true);
			Essentials::globalAudioManager->StopMusic(SoundID::eMusicLevel2, true);
			Essentials::globalAudioManager->StopMusic(SoundID::eMusicLoop, true);
			Essentials::globalAudioManager->PlayMusic(SoundID::eMusicLevel3);
		}
	}
	else
	if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eMusicLoop))
	{
		Essentials::globalAudioManager->StopMusic(SoundID::eMusicLevel1, true);
		Essentials::globalAudioManager->StopMusic(SoundID::eMusicLevel3, true);
		Essentials::globalAudioManager->StopMusic(SoundID::eMusicLevel2, true);
		Essentials::globalAudioManager->PlayMusic(SoundID::eMusicLoop);
	}

	Essentials::globalAudioManager->Update(deltaTime);
	Essentials::PushGameObjectsInto(myGameObjects);
	mySceneTransitionController.Update(deltaTime);

	if (Essentials::GetPlayer())
	{
		if (!myGameObjects.empty())
		{
			if (Essentials::GetPlayer()->GetComponent<PauseMenuComponent>()->ReturnToMainMenu())
			{
				Essentials::globalAudioManager->StopAllEvents();
				return eState::ePopState;
			}
		}
	}

	if (mySceneName == "Levels/TransitionScene1.tgs")
	{
		return eState::eLoadFirstLog;
	}

	if (mySceneName == "Levels/TransitionScene2.tgs")
	{
		return eState::eLoadSecondLog;
	}

	if (mySceneName == "Levels/OutroScene.tgs")
	{
		return eState::eOutro;
	}

	return eState::COUNT;
}

void InGame::Render()
{
	mySceneRenderer.Render(
		myGameObjects,
		*myCameraSystem,
		myVfxSystem,
		myEnablePointLights,
		myEnableDirectionalLight,
		myEnableAmbientLight,
		true);
	myCombatSystem.RenderDebug();

	{
		Tga::DX11::BackBuffer->SetAsActiveTarget();
		auto* sprite = Essentials::GetPlayer()->GetComponent<StaticSpriteComponent>();
		if (sprite && sprite->IsEnabled())
		{
			sprite->Render();
		}
		Tga::DX11::BackBuffer->SetAsActiveTarget(Tga::DX11::DepthBuffer);
	}
	UICanvas::RenderAll();

	RenderSceneFadeOverlay();


#ifndef _RETAIL
	myCameraSystem->RenderDebugUi();
#endif
}

bool InGame::RequestSceneTransition(
	const std::string& aTargetScene,
	const std::string& aTargetSpawnId,
	const float aFadeOutSeconds)
{
	SceneTransitionController::Request request;
	request.targetScene = aTargetScene;
	request.targetSpawnId = aTargetSpawnId;
	request.fadeSeconds = aFadeOutSeconds;
	request.source = "WorldTransitionService";
	return mySceneTransitionController.RequestTransition(request);
}

void InGame::ApplyTransitionScene(
	std::vector<std::unique_ptr<GameObject>>&& someObjects,
	const std::string& aScenePath,
	const std::string& aTargetSpawnId)
{
	(void)aTargetSpawnId;
	ApplyLoadedScene(std::move(someObjects), aScenePath);
	RuntimeCollisionService::Set(&myRuntimeCollisionSystem, &myGameObjects);
	myRuntimeCollisionSystem.AuditRequiredColliders(myGameObjects);
}

void InGame::RenderSceneFadeOverlay()
{
	const float sceneFadeAlpha = mySceneTransitionController.GetFadeAlpha();
	if (sceneFadeAlpha <= 0.0f)
	{
		return;
	}

	Tga::Engine* engine = Tga::Engine::GetInstance();
	if (!engine)
	{
		return;
	}

	Tga::SpriteSharedData sharedData;
	sharedData.myTexture = engine->GetTextureManager().GetWhiteSquareTexture();

	const Tga::Vector2ui renderSize = engine->GetRenderSize();
	Tga::Sprite2DInstanceData instance;
	instance.myPosition = {
		static_cast<float>(renderSize.x) * 0.5f,
		static_cast<float>(renderSize.y) * 0.5f
	};
	instance.mySize = {
		static_cast<float>(renderSize.x),
		static_cast<float>(renderSize.y)
	};
	instance.myColor = { 0.0f, 0.0f, 0.0f, sceneFadeAlpha };
	instance.myRenderOrder = 10000;

	Tga::DX11::BackBuffer->SetAsActiveTarget();
	auto& graphicsStateStack = engine->GetGraphicsEngine().GetGraphicsStateStack();
	graphicsStateStack.Push();
	graphicsStateStack.SetDefaultCamera();
	engine->GetGraphicsEngine().GetSpriteDrawer().Draw(sharedData, instance);
	graphicsStateStack.Pop();
	Tga::DX11::BackBuffer->SetAsActiveTarget(Tga::DX11::DepthBuffer);
}

namespace
{
	void DispatchTriggerPhase(GameObject& anObject, CollisionPhase aPhase)
	{
		const bool isEnter = aPhase == CollisionPhase::Enter || aPhase == CollisionPhase::Stay;

		if (auto* box = anObject.GetComponent<BoxColliderComponent>())
		{
			if (box->IsTrigger())
			{
				isEnter ? box->OnTriggerEnter() : box->OnTriggerExit();
			}
		}

		if (auto* sphere = anObject.GetComponent<SphereColliderComponent>())
		{
			if (sphere->IsTrigger())
			{
				isEnter ? sphere->OnTriggerEnter() : sphere->OnTriggerExit();
			}
		}

		if (auto* capsule = anObject.GetComponent<CapsuleColliderComponent>())
		{
			if (capsule->IsTrigger())
			{
				isEnter ? capsule->OnTriggerEnter() : capsule->OnTriggerExit();
			}
		}

		if (auto* obb = anObject.GetComponent<ObbColliderComponent>())
		{
			if (obb->IsTrigger())
			{
				isEnter ? obb->OnTriggerEnter() : obb->OnTriggerExit();
			}
		}
	}
}

void InGame::ConsumeCollisionContacts(const std::vector<CollisionContact>& someContacts)
{
	for (const CollisionContact& contact : someContacts)
	{
		if (contact.first != nullptr)
		{
			contact.first->DispatchCollisionContact(contact);
			DispatchTriggerPhase(*contact.first, contact.phase);
		}

		if (contact.second != nullptr)
		{
			contact.second->DispatchCollisionContact(contact);
			DispatchTriggerPhase(*contact.second, contact.phase);
		}
	}
}
