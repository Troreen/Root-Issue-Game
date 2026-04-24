#pragma once

#include <tge/scene/SceneObjectDefinitionManager.h>
#include <tge/graphics/Camera.h>
#include <tge/animation/AnimationClip.h>
#include <tge/animation/AnimationPlayer.h>
#include <tge/script/Script.h>
#include <tge/script/ScriptRuntimeInstance.h>
#include <tge/model/Model.h>

#include "tge/model/AnimatedModelInstance.h"
#include "tge/model/ModelInstance.h"

namespace Tga 
{
	class InputManager;
	class Scene;
}

struct PlayerWithAnimationClips
{
	Tga::SceneObjectDefinition* objectDefinition;
	std::shared_ptr<const Tga::Model> model;

	Tga::AnimationClip idleClip;
	Tga::AnimationPlayer idleAnimationPlayer;
	Tga::AnimationClip jogClip;
	Tga::AnimationPlayer jogAnimationPlayer;

	Tga::AnimatedModelInstance playerModel;
};

struct PlayerWithAnimationScripts
{
	Tga::SceneObjectDefinition* objectDefinition;

	std::shared_ptr<const Tga::Script> idleScript;
	std::shared_ptr<const Tga::Script> locomotionScript;

	std::optional<Tga::ScriptRuntimeInstance> idleScriptInstance;
	std::optional<Tga::ScriptRuntimeInstance> locomotionScriptInstance;

	std::unordered_map<Tga::StringId, Tga::Property> dynamicProperties;
	std::unordered_map<Tga::StringId, Tga::Property> staticProperties;

	Tga::AnimatedModelInstance playerModel;

	Tga::StringId posePropertyName;

	float syncedTime = 0.f;

};

class GameWorld
{
public:
	GameWorld(); 
	~GameWorld();

	void Init();
	void Update(float aTimeDelta, Tga::InputManager& inputManager);
	void Render();
private:
	int myFrameNumber = 0;
	Tga::SceneObjectDefinitionManager mySceneObjectDefinitionManager;
	PlayerWithAnimationClips myPlayerWithAnimationClips;
	PlayerWithAnimationScripts myPlayerWithAnimationScripts;
};