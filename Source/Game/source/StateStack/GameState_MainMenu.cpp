#include "MainMenu.h"
#include <tge/animation/Script/AnimationNodes.h>
#include <tge/script/Nodes/CommonMathNodes.h>
#include <tge/script/Nodes/CommonNodes.h>

#include "MeshComponent.h"
#include "GameObject.h"

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
}

eState MainMenu::Update()
{
	myTimer.Update();
	myInputHandler.UpdateInput();
	const float deltaTime = myTimer.GetDeltaTime();


	myVfxSystem.Update(deltaTime);

	if (!Essentials::globalAudioManager->IsEventPlaying(SoundID::eMusicLoop))
	{
		Essentials::globalAudioManager->PlayMusic(SoundID::eMusicLoop);
	}
	Essentials::globalAudioManager->Update(deltaTime);
	return eState::COUNT;
}

void MainMenu::Render()
{
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
	RenderDefault();
}
