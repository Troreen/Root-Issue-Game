#pragma once

#include <iostream>
#include <tge/engine.h>
#include <tge/input/InputManager.h>
#include <tge/graphics/Camera.h>
#include "SceneManager.h"
#include "EnumKeys.h"
#include "AudioManager.h"
#include "CameraSystem.h"

class GameObject;

class Essentials // ONE TRUE GLOBAL
{
public:

	Essentials(const Essentials& someEssentials) = delete;
	Essentials& operator=(const Essentials& someEssentials) = delete;

	static Essentials& GetEssentials()
	{
		static Essentials anEssentials;

		return anEssentials;
	}


	static float GetTotalTime();
	static float GetDeltaTime();
	static float GetUnscaledTotalTime();
	static float GetUnscaledDeltaTime();
	static void Shutdown();
	static Tga::Vector2f GetResolution();
	static Tga::Vector2i GetResolutionInt();

	static GameObject* GetPlayer();
	static void SetPlayer(GameObject& aPlayer);

	static inline std::unique_ptr<Tga::InputManager> globalInputManager;
	static inline Tga::Engine* globalEngine;
	static inline std::unique_ptr<CameraSystem> globalCamera;
	static inline std::unique_ptr<SceneManager> globalSceneManager;


	static inline std::unique_ptr<AudioManager> globalAudioManager;

	static inline bool ShutdownQueued;

private:
	
	static inline GameObject* myPlayer;

	Essentials();
	~Essentials();

};



