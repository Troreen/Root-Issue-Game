#pragma once

#include <tge/scene/SceneObjectDefinitionManager.h>
#include <tge/graphics/Camera.h>

namespace Tga 
{
	class InputManager;
	class Scene;
}

extern int globalMaxSize;
void setglobalmax(int v);


class GameWorld
{
public:
	GameWorld(); 
	~GameWorld();

	void Init();
	void LoadScene(Tga::Scene& aScene);
	void Update(float aTimeDelta, Tga::InputManager& inputManager);
	void Render();
private:
	Tga::Camera myCamera;
	Tga::SceneObjectDefinitionManager mySceneObjectDefinitionManager;
};