#include "Log2.h"
#include "AudioManager.h"
#include "Essentials.h"

#include <tge/animation/Script/AnimationNodes.h>
#include <tge/input/InputManager.h>
#include <tge/script/Nodes/CommonMathNodes.h>
#include <tge/script/Nodes/CommonNodes.h>
#include <tge/sprite/sprite.h>
#include <tge/engine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/graphics/GraphicsEngine.h>

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

void LogTransitionTwo::Init(CameraSystem& aCamera, const char* argv[])
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
	myUICanvas.Init("TransitionCanvas2", *Essentials::globalCanvasManager);

	if (UIText* uiText = myUICanvas.GetText("UIText"))
	{
		uiText->tint = { 1,1,1,0 };
	}
	UIText* uiText = myUICanvas.GetText("UITarget");
	UIImage* uiImage = myUICanvas.GetImage("Background");
	uiText->tint.a = 0.0f;
	uiImage->tint.a = 0.0f;
	myCounter = 0.f;
	myNextCharTimer = 0.036f;

	myText = Tga::Text("Assets/Art/2D/UI/OCRAEXT.ttf", Tga::FontSize_36);
	UpdateTextRes();
	mySize = 0;
}

eState LogTransitionTwo::Update()
{
	myInputHandler.UpdateInput();
	for (int i = 0; i < static_cast<int>(Keys::OEM_CLEAR); i++)
	{
		if (Essentials::globalInputManager.get()->IsKeyPressed(i))
		{
			myUICanvas.SetIsHidden(true);
			return eState::ePopStack;
		}
	}


	UICanvas::UpdateAll();
	UIText* uiText = myUICanvas.GetText("UITarget");
	UIImage* uiImage = myUICanvas.GetImage("Background");
	if (!myWaits[0] && uiText->tint.a < 0.99f)
	{
		uiText->tint.a = uiText->tint.a + 0.01f;
		uiImage->tint.a = uiImage->tint.a + 0.01f;
		return eState::COUNT;
	}
	else if (!myWaits[0])
	{
		uiText->tint.a = 1.f;
		uiImage->tint.a = 1.f;
		myWaits[0] = true;
		return eState::COUNT;
	}


	myCounter += Essentials::GetDeltaTime();
	UIText* Text = myUICanvas.GetText("UIText");
	UpdateTextRes();
	int index = 0;
	for (index ; index < mySize; index++)
	{
		myText.SetText(myText.GetText() + Text->text[index]);
	}
	RandomFloat myRandomGenerator;
	if (Text->text[index] == *".")
	{
		myNextCharTimer = myRandomGenerator.GetRandomFloat(0.45f, 0.85f);
		
	}
	else
	{
		myNextCharTimer = myRandomGenerator.GetRandomFloat(0.018f, 0.099f);
	}

	if (myCounter > myNextCharTimer)
	{
		myCounter = 0.f;
		if (mySize < sizeof(Text->text))
		{
			mySize++;
		}
	}
	UIText* Text2 = myUICanvas.GetText("UITarget");

	if (Text2 && Text2->text != myText.GetText().c_str())
	{
		std::snprintf(Text2->text,
			sizeof(Text2->text),
			"%s",
			myText.GetText().c_str());

		Text2->textObject.SetText(Text2->text);
	}

	return eState::COUNT;
}

void LogTransitionTwo::Render()
{
	/*myText.Render();*/
	UICanvas::RenderAll();
}

void LogTransitionTwo::UpdateTextRes()
{
	if (UIText* Text = myUICanvas.GetText("UIText"))
	{
		myText.SetScale(Text->fontScale);
		myText.SetText(Text->text);
	}
	Tga::Vector2ui screenres = Tga::Engine::GetInstance()->GetRenderSize();
	myText.SetPosition(Tga::Vector2f(myText.GetPosition().x, static_cast<float>(screenres.y) - myText.GetHeightWithoutLines()));
	myText.SetText("");
}

