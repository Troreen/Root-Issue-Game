#include "Essentials.h"

Essentials::Essentials()
{
	globalEngine = Tga::Engine::GetInstance();

	globalInputManager = std::make_unique<Tga::InputManager>(*globalEngine->GetHWND());

	globalSceneManager = std::make_unique<SceneManager>();

	globalCamera = std::make_unique<CameraSystem>();

	globalAudioManager = std::make_unique<AudioManager>();

	globalConsoleManager = std::make_unique<Console>();

	ShutdownQueued = false;

	myCursor = std::make_unique<Cursor>();

	globalCanvasManager = std::make_unique<Tga::CanvasObjectDefinitionManager>();

	globalPostMaster = std::make_unique <PostMaster>();
	globalAnimationEvents = std::make_unique<AnimationEventService>();

	myDynamicGameObjects = std::make_unique<std::vector<std::unique_ptr<GameObject>>>();

	myEnemiesToSpawn = std::make_unique<std::vector<EnemyAIComponent*>>();
}

Essentials::~Essentials()
{
}

float Essentials::GetTotalTime()
{
	return  globalEngine->GetTotalTime();
}

float Essentials::GetDeltaTime()
{
	return globalEngine->GetDeltaTime();
}

float Essentials::GetUnscaledTotalTime()
{
	return globalEngine->GetUnscaledTotalTime();
}

float Essentials::GetUnscaledDeltaTime()
{
	return globalEngine->GetUnscaledDeltaTime();
}

void Essentials::Shutdown()
{
	ShutdownQueued = true;
}

Tga::Vector2f Essentials::GetResolution()
{
	Tga::Vector2ui res = globalEngine->GetRenderSize();
	return { static_cast<float>(res.x), static_cast<float>(res.y) };
}

Tga::Vector2i Essentials::GetResolutionInt()
{
	Tga::Vector2ui res = globalEngine->GetRenderSize();
	return { static_cast<int>(res.x), static_cast<int>(res.y) };
}

void Essentials::AddGameObject(std::unique_ptr<GameObject>& aGameObject)
{
	myDynamicGameObjects->push_back(std::move(aGameObject));
}

void Essentials::PushGameObjectsInto(std::vector<std::unique_ptr<GameObject>>& outGameobjects)
{
	outGameobjects.insert(outGameobjects.end(), std::make_move_iterator(myDynamicGameObjects->begin()), std::make_move_iterator(myDynamicGameObjects->end()));
	myDynamicGameObjects->clear();
}

void Essentials::AddEnemy(EnemyAIComponent* aEnemy)
{
	myEnemiesToSpawn->push_back(aEnemy);
}

void Essentials::PushEnemyInto(std::vector<EnemyAIComponent*>& outGameobjects)
{
	outGameobjects.insert(outGameobjects.end(), myEnemiesToSpawn->begin(), myEnemiesToSpawn->end());
	myEnemiesToSpawn->clear();
}

GameObject* Essentials::GetPlayer()
{
	return myPlayer;
}

void Essentials::SetPlayer(GameObject& aPlayer)
{
	myPlayer = &aPlayer;
}
