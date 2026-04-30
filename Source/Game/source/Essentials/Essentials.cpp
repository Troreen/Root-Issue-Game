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

GameObject* Essentials::GetPlayer()
{
	return myPlayer;
}

void Essentials::SetPlayer(GameObject& aPlayer)
{
	myPlayer = &aPlayer;
}
