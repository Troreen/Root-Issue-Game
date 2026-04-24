#pragma once

#include <unordered_set>
#include <tge/scene/SceneObjectDefinitionManager.h>
#include <tge/animation/AnimationClip.h>
#include <tge/animation/AnimationPlayer.h>
#include <tge/script/Script.h>
#include <tge/script/ScriptRuntimeInstance.h>
#include <tge/sprite/sprite.h>
#include <tge/text/text.h>

#include "tge/Engine.h"


namespace Tga
{
	class Scene;
	class InputManager;
}

struct Entity
{
    Tga::StringId definitionName;
    Tga::StringId name;
    Tga::Vector2i gridPos;
    Tga::Vector2f offset;

    bool isVisible = true;
    Tga::StringId activeSpriteName;
    struct Sprite
    {
        Tga::StringId name;
        Tga::SpriteSharedData spriteShared = {};
        Tga::Sprite2DInstanceData spriteInstance = {};
    };
    std::vector<Sprite> sprites;

    std::unordered_map<Tga::StringId, Tga::Property> dynamicProps;
    std::unordered_map<Tga::StringId, Tga::Property> staticProps;

    std::vector<std::shared_ptr<const Tga::Script>> scripts;
    std::vector<std::optional<Tga::ScriptRuntimeInstance>> scriptInstances;
};


class GameWorld
{
public:
    void Init();
    void LoadScene(Tga::Scene& scene);
    void Update(float dt, Tga::InputManager& input);
    void Render();

    bool IsPlatePressed(Tga::StringId name) const { return myPressedPressurePlates.contains(name); }
    bool WasPlatePressed(Tga::StringId name) const { return myPressedPressurePlatesPrevFrame.contains(name); }

    bool IsPlayerNearby(Tga::Vector2i position) const
    {
        Tga::Vector2i diff = myPlayer->gridPos - position;
        return abs(diff.x) <= 1 && abs(diff.y) <= 1;
    }

    void SetMessage(Tga::StringId message) { myMessageString = message; }

private:
    std::vector<std::unique_ptr<Entity>> myEntities;

    Entity* myPlayer = nullptr;
    std::vector<Entity*> myBalls;
    std::vector<Entity*> myBackgroundEntities;

    Tga::SceneObjectDefinitionManager mySceneDefinitionManager;
    int myFrameNumber = 0;

    std::unordered_set<Tga::StringId> myPressedPressurePlates;
    std::unordered_set<Tga::StringId> myPressedPressurePlatesPrevFrame;

    Tga::StringId myMessageString;
    Tga::Text myMessageText;
};
