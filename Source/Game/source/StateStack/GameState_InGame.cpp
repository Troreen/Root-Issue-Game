#include "InGame.h"
#include <tge/animation/Script/AnimationNodes.h>
#include <tge/script/Nodes/CommonMathNodes.h>
#include <tge/script/Nodes/CommonNodes.h>

#include "MeshComponent.h"
#include "GameObject.h"
#include "BoxColliderComponent.h"
#include "CapsuleColliderComponent.h"
#include "ObbColliderComponent.h"
#include "PickUpComponent.h"
#include "SphereColliderComponent.h"

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

	bool IsTriggerContactObject(const GameObject& anObject)
	{
		if (anObject.GetLayer() == ObjectLayer::Trigger || anObject.GetLayer() == ObjectLayer::Pickup)
		{
			return true;
		}

		if (const auto* box = anObject.GetComponent<BoxColliderComponent>())
		{
			return box->IsTrigger();
		}

		if (const auto* sphere = anObject.GetComponent<SphereColliderComponent>())
		{
			return sphere->IsTrigger();
		}

		if (const auto* capsule = anObject.GetComponent<CapsuleColliderComponent>())
		{
			return capsule->IsTrigger();
		}

		if (const auto* obb = anObject.GetComponent<ObbColliderComponent>())
		{
			return obb->IsTrigger();
		}

		return false;
	}

	void DispatchTriggerPhase(GameObject& anObject, const CollisionPhase aPhase)
	{
		if (!IsTriggerContactObject(anObject))
		{
			return;
		}

		if (auto* box = anObject.GetComponent<BoxColliderComponent>())
		{
			if (aPhase == CollisionPhase::Enter)
			{
				box->OnTriggerEnter();
			}
			else if (aPhase == CollisionPhase::Exit)
			{
				box->OnTriggerExit();
			}
		}

		if (auto* sphere = anObject.GetComponent<SphereColliderComponent>())
		{
			if (aPhase == CollisionPhase::Enter)
			{
				sphere->OnTriggerEnter();
			}
			else if (aPhase == CollisionPhase::Exit)
			{
				sphere->OnTriggerExit();
			}
		}

		if (auto* capsule = anObject.GetComponent<CapsuleColliderComponent>())
		{
			if (aPhase == CollisionPhase::Enter)
			{
				capsule->OnTriggerEnter();
			}
			else if (aPhase == CollisionPhase::Exit)
			{
				capsule->OnTriggerExit();
			}
		}

		if (auto* obb = anObject.GetComponent<ObbColliderComponent>())
		{
			if (aPhase == CollisionPhase::Enter)
			{
				obb->OnTriggerEnter();
			}
			else if (aPhase == CollisionPhase::Exit)
			{
				obb->OnTriggerExit();
			}
		}
	}

	GameObject* FindPickupObject(const CollisionContact& aContact)
	{
		if (aContact.first && (aContact.first->GetLayer() == ObjectLayer::Pickup || aContact.first->GetComponent<PickUpComponent>()))
		{
			return aContact.first;
		}

		if (aContact.second && (aContact.second->GetLayer() == ObjectLayer::Pickup || aContact.second->GetComponent<PickUpComponent>()))
		{
			return aContact.second;
		}

		return nullptr;
	}
}


void InGame::Init(CameraSystem& aCamera, const char* argv[])
{
	Tga::Engine& engine = *Tga::Engine::GetInstance();
	myLoadingText = { "Text/Evil Bible.ttf", Tga::FontSize_72 };
	myCameraSystem = &aCamera;
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

	RegisterAnimationGraphNodesOnce();
	myCameraSystem->Init();

	RegisterGameObjectFactories();

	engine;

    /*myInputHandler.SetWindowHandle(*engine.GetHWND());
	myInputHandler.SetAutoMouseCapture(true);*/

	mySceneName.clear();
	myCameraSystem->SetSceneName(mySceneName);

	myVfxSystem.Init();
	VfxService::Set(&myVfxSystem);

	/*SceneManager& sceneManager = *Essentials::globalSceneManager;
	sceneManager.RefreshSceneList();*/

	if (argv[1] == nullptr)
	{
		StartSceneLoadAsync("Levels/TestScenes/PabloTestingScene.tgs", true);
	}
	else
	{
		StartSceneLoadAsync(argv[1], true);
	}
}

eState InGame::Update()
{
	myTimer.Update();
	myInputHandler.UpdateInput();
	TryRecoverWindowFocus();
	const float deltaTime = myTimer.GetDeltaTime();

	//PollSceneLoadCompletion();

	SceneManager& sceneManager = *Essentials::globalSceneManager;
	if (sceneManager.HasRequestedScene())
	{
		const std::string requested = sceneManager.ConsumeRequestedScene();

		if (!requested.empty() && requested != mySceneLoadTarget && requested != mySceneName)
		{
			if (myIsSceneLoading)
			{
				myQueuedSceneRequest = requested;
			}
			else
			{
				//StartSceneLoadAsync(requested);
			}
		}
	}

	if (myIsSceneLoading)
	{
		return eState::COUNT;
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

	myCameraSystem->UpdateDebugCamera(deltaTime, myInputHandler);
	/*myCameraSystem->Update(deltaTime);*/

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

	myRuntimeCollisionSystem.Run(myGameObjects);
	ConsumeCollisionContacts(myRuntimeCollisionSystem.GetContacts());

	myVfxSystem.Update(deltaTime);

	if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eVineBoom))
	{
		Essentials::globalAudioManager->PlayMusic(SoundID::eVineBoom);
	}

	Essentials::globalAudioManager->Update(deltaTime);
	return eState::COUNT;
}

void InGame::Render()
{
	if (myIsSceneLoading)
	{
		RenderLoadingScreen();
		return;
	}

	RenderDefault();
}

void InGame::OnSceneLoaded()
{
	myRuntimeCollisionSystem.AuditRequiredColliders(myGameObjects);
}

void InGame::ConsumeCollisionContacts(const std::vector<CollisionContact>& someContacts)
{
	for (const CollisionContact& contact : someContacts)
	{
		if (!contact.first || !contact.second)
		{
			continue;
		}

		if (contact.phase == CollisionPhase::Enter || contact.phase == CollisionPhase::Exit)
		{
			DispatchTriggerPhase(*contact.first, contact.phase);
			DispatchTriggerPhase(*contact.second, contact.phase);
		}

		if (contact.phase == CollisionPhase::Enter || contact.phase == CollisionPhase::Stay)
		{
			GameObject* pickup = FindPickupObject(contact);
			if (pickup && pickup->IsActive())
			{
				pickup->RemoveAllComponents();
				pickup->SetActive(false);
			}
		}
	}
}
