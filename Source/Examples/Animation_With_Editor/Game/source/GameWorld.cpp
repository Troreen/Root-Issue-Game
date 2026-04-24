#include "GameWorld.h"

#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/texture/TextureManager.h>
#include <tge/animation/AnimationPlayer.h>
#include <tge/drawers/DebugDrawer.h>

#include <tge/scene/Scene.h>
#include <tge/graphics/DX11.h>
#include <tge/graphics/RenderTarget.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/drawers/ModelDrawer.h>
#include <tge/graphics/Camera.h>
#include <tge/script/BaseProperties.h>
#include <tge/script/Contexts/ScriptUpdateContext.h>
#include <tge/scene/ScenePropertyTypes.h>
#include <tge/settings/settings.h>
#include <tge/math/Matrix4x4.h>
#include <tge/input/InputManager.h>
#include <tge/Timer.h>

#include <tge/script/ScriptManager.h>
#include <tge/script/Nodes/CommonNodes.h>
#include <tge/script/Nodes/CommonMathNodes.h>
#include <tge/animation/Script/AnimationNodes.h>

#include "tge/math/Fmath.h"



using namespace Tga;

GameWorld::GameWorld()
{}

GameWorld::~GameWorld() 
{}

typedef void (*ActiveFunc)(bool aIsActive);

void GameWorld::Init()
{
	// before 
	RegisterCommonNodes();
	RegisterCommonMathNodes();
	RegisterAnimationNodes();

	mySceneObjectDefinitionManager.Init(Settings::GameAssetRoot().string().c_str());

	// loading a TGO together with some animation clips
	// this example plays animations directly, not controlled by scripting
	{
		myPlayerWithAnimationClips.objectDefinition = mySceneObjectDefinitionManager.Get("player_with_clips"_tgaid);
		std::span<const ScenePropertyDefinition> objectProperties = myPlayerWithAnimationClips.objectDefinition->GetProperties();

		for (const ScenePropertyDefinition& property : objectProperties)
		{
			if (property.type == GetPropertyType<CopyOnWriteWrapper<SceneModel>>())
			{
				const SceneModel& value = property.value.Get<CopyOnWriteWrapper<SceneModel>>()->Get();
				StringId path = value.path;

				myPlayerWithAnimationClips.playerModel = ModelFactory::GetInstance().GetAnimatedModelInstance(path.GetString());
			}
		}

		for (const ScenePropertyDefinition& property : objectProperties)
		{
			if (property.name == "clip_idle"_tgaid)
			{
				StringId path = property.value.Get<CopyOnWriteWrapper<AnimationClipReference>>()->Get().path;
				myPlayerWithAnimationClips.idleClip = *GetAnimationClip(path);
				myPlayerWithAnimationClips.idleAnimationPlayer = ModelFactory::GetInstance().GetAnimationPlayer(myPlayerWithAnimationClips.idleClip.animationSourcePath.GetString(), myPlayerWithAnimationClips.playerModel.GetModel());
			}

			if (property.name == "clip_jog"_tgaid)
			{
				StringId path = property.value.Get<CopyOnWriteWrapper<AnimationClipReference>>()->Get().path;
				myPlayerWithAnimationClips.jogClip = *GetAnimationClip(path);
				myPlayerWithAnimationClips.jogAnimationPlayer = ModelFactory::GetInstance().GetAnimationPlayer(myPlayerWithAnimationClips.jogClip.animationSourcePath.GetString(), myPlayerWithAnimationClips.playerModel.GetModel());
			}
		}
	}

	// load a TGO together with animation graphs
	// this example runs graphs, allowing custom logic for blending and adjusting animations
	{
		myPlayerWithAnimationScripts.objectDefinition = mySceneObjectDefinitionManager.Get("player_with_scripts"_tgaid);
		std::span<const ScenePropertyDefinition> objectProperties = myPlayerWithAnimationScripts.objectDefinition->GetProperties();

		// this loads the script nodes, the script "code"
		myPlayerWithAnimationScripts.idleScript = ScriptManager::GetScript("PlayerWithScripts/player_with_scripts_idle");
		myPlayerWithAnimationScripts.locomotionScript = ScriptManager::GetScript("PlayerWithScripts/player_with_scripts_locomotion");

		// this setups up a runnable instance of the script (which can store live value)
		myPlayerWithAnimationScripts.idleScriptInstance = ScriptRuntimeInstance(myPlayerWithAnimationScripts.idleScript);
		myPlayerWithAnimationScripts.idleScriptInstance->Init();

		myPlayerWithAnimationScripts.locomotionScriptInstance = ScriptRuntimeInstance(myPlayerWithAnimationScripts.locomotionScript);
		myPlayerWithAnimationScripts.locomotionScriptInstance->Init();

		for (const ScenePropertyDefinition& property : objectProperties)
		{
			// take the model and prepare for drawing:
			if (property.type == GetPropertyType<CopyOnWriteWrapper<SceneModel>>())
			{
				const SceneModel& value = property.value.Get<CopyOnWriteWrapper<SceneModel>>()->Get();
				StringId path = value.path;

				myPlayerWithAnimationScripts.playerModel = ModelFactory::GetInstance().GetAnimatedModelInstance(path.GetString());

				// The animation graph creates its own properties for storing results, we store their names so we don't have to construct them each frame:
				std::string poseNameString = property.name.GetString();
				poseNameString += "_pose";
				myPlayerWithAnimationScripts.posePropertyName = StringRegistry::RegisterOrGetString(poseNameString);
			}

			// This reads the properties from the TGO, and sets them up for usage with the scripts.
			if ((property.flags & ScenePropertyFlags::IsDynamic) != ScenePropertyFlags::None)
			{
				// These properties are dynamic and can be changed per instance of the TGO
				// If you have multiple TGOs with scripts, each one needs a copy of these:
				myPlayerWithAnimationScripts.dynamicProperties[property.name] = property.value;
			}
			else
			{
				// This properties will not change
				// If there are multiple instances of the same TGO, these can be shared
				myPlayerWithAnimationScripts.staticProperties[property.name] = property.value;
			}
		}
	}
}

void GameWorld::Update(float aTimeDelta, InputManager& inputManager)
{
	Vector2f moveDir = {0.f, 0.f};

	// right now we're very abruptly changing direction
	// for more fluid motion, do some kind of acceleration instead!

	if (inputManager.IsKeyHeld('W'))
		moveDir += {0.f, -1.f};
	if (inputManager.IsKeyHeld('S'))
		moveDir += { 0.f, 1.f };

	if (inputManager.IsKeyHeld('A'))
		moveDir += { 1.f, 0.f };
	if (inputManager.IsKeyHeld('D'))
		moveDir += { -1.f, 0.f };

	myPlayerWithAnimationClips.playerModel.GetTransform().SetPosition(Vector3f{ 200.f, 0.f, 0.f });
	myPlayerWithAnimationScripts.playerModel.GetTransform().SetPosition(Vector3f{ -200.f, 0.f, 0.f });

	bool isRunning = false;
	if (moveDir.LengthSqr() > 1e-3f)
	{
		isRunning = true;
		float angle = atan2(moveDir.x, moveDir.y);

		myPlayerWithAnimationClips.playerModel.GetTransform().SetRotation(Vector3f(0.f, angle * FMath::RadToDeg, 0.f));
		myPlayerWithAnimationScripts.playerModel.GetTransform().SetRotation(Vector3f(0.f, angle * FMath::RadToDeg, 0.f));
	}

	// character set up with clips
	{
		if (isRunning)
		{
			if (myPlayerWithAnimationClips.jogAnimationPlayer.GetState() != AnimationState::Playing)
				myPlayerWithAnimationClips.jogAnimationPlayer.Play();

			myPlayerWithAnimationClips.jogAnimationPlayer.Update(myPlayerWithAnimationClips.jogClip, aTimeDelta);
			myPlayerWithAnimationClips.playerModel.SetPose(myPlayerWithAnimationClips.jogAnimationPlayer.GetLocalSpacePose());
		}
		else
		{
			if (myPlayerWithAnimationClips.idleAnimationPlayer.GetState() != AnimationState::Playing)
				myPlayerWithAnimationClips.idleAnimationPlayer.Play();

			myPlayerWithAnimationClips.idleAnimationPlayer.Update(myPlayerWithAnimationClips.idleClip, aTimeDelta);
			myPlayerWithAnimationClips.playerModel.SetPose(myPlayerWithAnimationClips.idleAnimationPlayer.GetLocalSpacePose());
		}
	}

	// character set up with animation graphs
	{


		myFrameNumber++;

		ScriptUpdateContext scriptUpdateContext;

		scriptUpdateContext.deltaTime = aTimeDelta;
		scriptUpdateContext.frameNumber = myFrameNumber;
		scriptUpdateContext.dynamicProperties = &myPlayerWithAnimationScripts.dynamicProperties;
		scriptUpdateContext.staticProperties = &myPlayerWithAnimationScripts.staticProperties;

		// We know there is a property set up in the TGO, that we in code can use to drive the animation graph
		// We change it in code, and the graphs read it to drive animation behavior
		// Have a dialogue in your game teams about what properties are needed in the graphs, then drive them like this:
		// You can also communicate the other way, you can write values in scripts and read them from code
		Property& walkJogBlend = myPlayerWithAnimationScripts.dynamicProperties["walk_jog_blend"_tgaid];

		if (isRunning)
		{

			// Accelerate from walking to running:
			// For now only playing the animation, but in a real application the player would of course also move!
			float previousValue = *walkJogBlend.Get<float>();

			if (previousValue == 0.f) // we're not moving already, reset the walk cycle to the beginning when starting to run
				myPlayerWithAnimationScripts.syncedTime = 0.f;

			constexpr float CHANGE_SPEED = 0.30f;
			*walkJogBlend.Get<float>() = 1.0f - (1.0f - previousValue) * std::pow(CHANGE_SPEED, aTimeDelta);

			myPlayerWithAnimationScripts.locomotionScriptInstance->Update(scriptUpdateContext);
		}
		else
		{
			// Reset walk to run transition since we have stopped
			*walkJogBlend.Get<float>() = 0.f;

			myPlayerWithAnimationScripts.idleScriptInstance->Update(scriptUpdateContext);
		}

		auto it = myPlayerWithAnimationScripts.dynamicProperties.find(myPlayerWithAnimationScripts.posePropertyName);
		if (it != myPlayerWithAnimationScripts.dynamicProperties.end())
		{
			PoseAndMotion* poseAndMotion = it->second.Get<PoseAndMotion>();

			if (poseAndMotion && poseAndMotion->poseGenerator)
			{
				// todo: this logic updates synced time (when animation graphs sync time between scripts)
				// needs to be stored and updated like this for correct playback
				// should have this in some shared function instead of needing it everywhere
				if (poseAndMotion->desiredSyncedPlaybackRateWeight > 0.f)
				{
					myPlayerWithAnimationScripts.syncedTime += poseAndMotion->desiredSyncedPlaybackRate * aTimeDelta;
					myPlayerWithAnimationScripts.syncedTime -= floor(myPlayerWithAnimationScripts.syncedTime);
				}

				// the graph sets up pose generation, but doesn't actually generate it
				// this is for flexibility when setting up more complex generation steps 
				PoseGenerationContext context = {};
				context.deltaTime = aTimeDelta;
				context.frameNumber = myFrameNumber;
				context.syncedPlaybackTime = myPlayerWithAnimationScripts.syncedTime;
				context.model = myPlayerWithAnimationScripts.playerModel.GetModel();

				LocalSpacePose pose;
				poseAndMotion->poseGenerator->GeneratePose(context, pose);

				myPlayerWithAnimationScripts.playerModel.SetPose(pose);
			}

			// removing pose every frame in case a script is stopped and the pose remains
			// otherwise we could get a dangling pointer
			myPlayerWithAnimationScripts.dynamicProperties.erase(it);
		}
	}
}

void GameWorld::Render()
{
	auto& engine = *Tga::Engine::GetInstance();
	auto& graphicsStateStack = engine.GetGraphicsEngine().GetGraphicsStateStack();

	Vector2ui resolution = engine.GetRenderSize();

	Camera camera = {};
	camera.SetPerspectiveProjection(
		90,
		{
			(float)resolution.x,
			(float)resolution.y
		},
		0.1f,
		50000.0f);

	camera.GetTransform().SetPosition(Vector3f(0.0f, 500.0f, -550.0f));
	Rotator camRotation = Rotator(45, 0, 0);
	camera.GetTransform().SetRotation(camRotation);

	graphicsStateStack.SetCamera(camera);

	DirectionalLight directionalLight;
	directionalLight.color = Color(1.f, 1.f, 1.f);
	Rotator lightRotation = Rotator(-45, -130, 0);
	directionalLight.transform.SetRotation(lightRotation);
	graphicsStateStack.SetDirectionalLight(directionalLight);

	AmbientLight ambientLight;
	ambientLight.color = Color(0.25f, 0.50f, 0.75f);
	graphicsStateStack.SetAmbientLight(ambientLight);

	DX11::BackBuffer->SetAsActiveTarget(DX11::DepthBuffer); // use depth buffer for 3D rendering
	graphicsStateStack.SetBlendState(Tga::BlendState::Disabled);
	Tga::Engine::GetInstance()->GetGraphicsEngine().GetModelDrawer().DrawLambert(myPlayerWithAnimationClips.playerModel);
	Tga::Engine::GetInstance()->GetGraphicsEngine().GetModelDrawer().DrawLambert(myPlayerWithAnimationScripts.playerModel);

}