#include "GameWorld.h"

#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/texture/TextureManager.h>

#include <tge/scene/Scene.h>
#include <tge/graphics/DX11.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/graphics/Camera.h>
#include <tge/script/BaseProperties.h>
#include <tge/script/Contexts/ScriptUpdateContext.h>
#include <tge/scene/ScenePropertyTypes.h>
#include <tge/settings/settings.h>
#include <tge/math/Matrix4x4.h>
#include <tge/input/InputManager.h>
#include <tge/Timer.h>

#include <tge/script/ScriptManager.h>
#include <tge/Engine.h>
#include <tge/math/Vector2.h>
#include <cmath>

#include "Nodes.h"
#include "tge/script/Nodes/CommonMathNodes.h"
#include "tge/script/Nodes/CommonNodes.h"
#include "tge/text/TextService.h"


using namespace Tga;

static void InitScripts(Entity& entity)
{
    entity.scriptInstances.clear();
    for (auto& s : entity.scripts)
    {
        entity.scriptInstances.emplace_back(Tga::ScriptRuntimeInstance(s));
        entity.scriptInstances.back()->Init();
    }
}

static void UpdateScripts(Entity& entity, Tga::ScriptUpdateContext& ctx)
{
    for (auto& inst : entity.scriptInstances)
        if (inst) inst->Update(ctx);
}

static void Render(Entity& entity)
{
    if (!entity.isVisible)
        return;

    for (Entity::Sprite& sprite : entity.sprites)
    {
        if (entity.activeSpriteName.IsEmpty() || entity.activeSpriteName == sprite.name)
        {
            sprite.spriteInstance.myPosition = 100.0f * Tga::Vector2f((float)entity.gridPos.x, (float)entity.gridPos.y) + entity.offset;

            Tga::Engine::GetInstance()->GetGraphicsEngine().GetSpriteDrawer()
                .Draw(sprite.spriteShared, sprite.spriteInstance);
        }
    }
}

static void DiscoverScriptsForEntityFromDefinition(Entity& entity, Tga::SceneObjectDefinition* objectDefinition)
{
    std::string relPath = objectDefinition->GetPath(); 
    std::filesystem::path fullPath =
        std::filesystem::path(Tga::Settings::GameAssetRoot()) / relPath;

    std::filesystem::path folder = fullPath.parent_path();
	std::string prefix = std::string(entity.definitionName.GetString()) + "_";

    for (auto& entry : std::filesystem::directory_iterator(folder))
    {
        if (!entry.is_regular_file())
            continue;

        std::string filename = entry.path().filename().string();

        if (filename.rfind(prefix, 0) != 0)
            continue;

        std::filesystem::path relScriptPath =
            std::filesystem::relative(entry.path(),
                std::filesystem::path(Tga::Settings::GameAssetRoot()));
        relScriptPath.replace_extension();

        std::string scriptId = relScriptPath.string();
        std::replace(scriptId.begin(), scriptId.end(), '\\', '/');

        auto script = Tga::ScriptManager::GetScript(scriptId.c_str());
        if (script)
        {
            entity.scripts.push_back(script);
        }
    }
}

void GameWorld::Init()
{
    RegisterCommonNodes();
    RegisterCommonMathNodes();
    RegisterGameNodes();

    mySceneDefinitionManager.Init(Tga::Settings::GameAssetRoot().string().c_str());
}

void GameWorld::LoadScene(Tga::Scene& scene)
{
    for (auto& pair : scene.GetSceneObjects())
    {
        auto* sceneObj = pair.second.get();

        std::vector<ScenePropertyDefinition> props;
        sceneObj->CalculateCombinedPropertySet(mySceneDefinitionManager, props);

        auto entity = std::make_unique<Entity>();
        entity->name = StringRegistry::RegisterOrGetString(sceneObj->GetName());
        entity->definitionName = pair.second->GetSceneObjectDefinitionName();

        Vector3f pos = sceneObj->GetTransform().GetPosition();
        entity->gridPos =
        {
            (int)std::round(pos.x / 100.f),
            (int)std::round(pos.z / 100.f)
        };

        for (auto& p : props)
        {
            if (p.type == GetPropertyType<CopyOnWriteWrapper<SceneSprite>>())
            {
                const SceneSprite& spr =
                    p.value.Get<CopyOnWriteWrapper<SceneSprite>>()->Get();

                Entity::Sprite sprite = {};

                sprite.name = p.name;
                sprite.spriteShared.myTexture =
                    Engine::GetInstance()->GetTextureManager()
                    .GetTexture(spr.textures[0].GetString());

                sprite.spriteInstance.mySize = spr.size;

                entity->sprites.push_back(sprite);
            }
        }

        for (auto& p : props)
        {
            if ((p.flags & ScenePropertyFlags::IsDynamic) != ScenePropertyFlags::None)
                entity->dynamicProps[p.name] = p.value;
            else
                entity->staticProps[p.name] = p.value;
        }

        SceneObjectDefinition* objectDefinition = mySceneDefinitionManager.Get(sceneObj->GetSceneObjectDefinitionName());

        DiscoverScriptsForEntityFromDefinition(*entity, objectDefinition);
        InitScripts(*entity);

        StringId defName = sceneObj->GetSceneObjectDefinitionName();

        if (defName == "pressure_plate"_tgaid)
            entity->offset = Vector2f(0.f, -100.f * 3.f/12.f);

        if (defName == "player"_tgaid)
        {
            myPlayer = entity.get();
        }
        else if (defName == "ball"_tgaid)
        {
            myBalls.push_back(entity.get());
        }
        else
        {
            myBackgroundEntities.push_back(entity.get());
        }

        myEntities.push_back(std::move(entity));
    }
}

void GameWorld::Update(float dt, InputManager& input)
{
    Vector2i move = { 0,0 };

    if (input.IsKeyPressed('W')) move.y += 1;
    if (input.IsKeyPressed('S')) move.y -= 1;
    if (input.IsKeyPressed('A')) move.x -= 1;
    if (input.IsKeyPressed('D')) move.x += 1;

    // Player movement
    if (move.x || move.y)
    {
        Vector2i target = myPlayer->gridPos + move;

        // Check if a ball needs to be pushed
        // In a real game it should check that the new location is empty!
        for (Entity* ball : myBalls)
        {
            if (ball->gridPos == target)
            {
                ball->gridPos = ball->gridPos + move;
            }
        }

        myPlayer->gridPos = target;
    }

    // Pressure plates
    std::swap(myPressedPressurePlates, myPressedPressurePlatesPrevFrame);
    myPressedPressurePlates.clear();

    for (auto& entity : myEntities)
    {
        StringId defName = entity->definitionName;

        if (defName != "pressure_plate"_tgaid)
            continue;

        bool isPressed = false;

        if (myPlayer && myPlayer->gridPos == entity->gridPos)
            isPressed = true;

        for (Entity* ball : myBalls)
        {
	        if (ball->gridPos == entity->gridPos)
	        {
                isPressed = true;
                break;
	        }
        }

        if (isPressed)
        {
            entity->activeSpriteName = "sprite_down"_tgaid;
            myPressedPressurePlates.insert(entity->name);
        }
        else
        {
            entity->activeSpriteName = "sprite_up"_tgaid;
        }
    }

    // run all scripts, setting up a context with all information nodes need access to:

    GameScriptUpdateContext context;
    context.deltaTime = dt;
    context.frameNumber = myFrameNumber;
    context.gameWorld = this;
 
    for (auto& entity : myEntities)
    {
        // if the script has a property called entity_visibility, use it to control entity visibility
        Property* visibilityProperty = nullptr;
        auto visIt = entity->dynamicProps.find("entity_visibility"_tgaid);
        if (visIt != entity->dynamicProps.end())
        {
            Property& p = visIt->second;
            if (!p.IsOfType<bool>())
            {
                std::cout << "Error, entity_visibility property should be of type bool \n";
            }
            else
            {
                visibilityProperty = &p;
            }
        }

        // if the script has a property called entity_position, use it to communicate position with the script
        Property* posProperty = nullptr;
        auto posIt = entity->dynamicProps.find("entity_position"_tgaid);
        if (posIt != entity->dynamicProps.end())
        {
            Property& p = posIt->second;
            if (!p.IsOfType<Vector2f>())
            {
                std::cout << "Error, entity_position property should be of type Float2 \n";
            }
            else
            {
                posProperty = &p;
            }
        }

        if (posProperty)
        {
            (*posProperty) = Property::Create<Vector2f>(Vector2f{ (float)entity->gridPos.x, (float)entity->gridPos.y });
        }

        context.currentEntity = entity.get();
        context.dynamicProperties = &entity->dynamicProps;
        context.staticProperties = &entity->staticProps;

        UpdateScripts(*entity, context);

        // write position back (letting the player move the 
        if (posProperty)
        {
            entity->gridPos = Vector2f(round(posProperty->Get<Vector2f>()->x), round(posProperty->Get<Vector2f>()->y));
        }

        if (visibilityProperty)
        {
            entity->isVisible = *visibilityProperty->Get<bool>();
        }
    }

    myFrameNumber++;
}

void GameWorld::Render()
{
    GraphicsStateStack& graphicsStateStack = Tga::Engine::GetInstance()->GetGraphicsEngine().GetGraphicsStateStack();

    Vector2ui halfres = DX11::GetResolution() / 2;
    Camera camera = {};
    camera.SetOrtographicProjection(
        -(float)halfres.x, (float)halfres.x, -(float)halfres.y, (float)halfres.y, -1.0f, 1.f);
    camera.SetTransform({});
    graphicsStateStack.SetCamera(camera);

    graphicsStateStack.SetSamplerState(SamplerFilter::Point, SamplerAddressMode::Clamp);

    for (auto& entity : myBackgroundEntities)
    {
        ::Render(*entity);
    }

    for (auto& entity : myBalls)
    {
        ::Render(*entity);
    }

    ::Render(*myPlayer);

    myMessageText.SetText(myMessageString.GetString());
    myMessageText.SetPosition({ (float)0.f, -0.75f*(float)halfres.y });
    myMessageText.Render();
}
