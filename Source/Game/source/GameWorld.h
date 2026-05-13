#pragma once

#include <CommonUtilities/InputHandler.h>
#include <CommonUtilities/Timer.h>

#include <string>

#include "CameraSystem.h"

#include "StateStack.hpp"

class GameObject;

/// Main game world class that manages all game objects. 
/// This is your main game container - add your game objects and logic here.
class GameWorld
{
public:
    GameWorld();
    ~GameWorld();

    /// Initialize the game world. Called once at startup.
    void Init(const char* argv[]);

    /// Update all game objects. Called every frame.
    void Update(float deltaTime , const char* argv[]);

    /// Render all game objects. Called every frame after Update.
    void Render();

    /// Get the input handler for processing input events.
    CommonUtilities::InputHandler& GetInputHandler();

    /// Get the timer for delta time and total elapsed time.
    CommonUtilities::Timer& GetTimer();

    // Get the main camera for the game world.
    CommonUtilities::Camera3Df* GetCamera();

    /// Get the current delta time (time since last frame in seconds).
    float GetDeltaTime() const;

private:
    static constexpr float ourFixedTimeStep = 1.0f / 60.0f;
    static constexpr int ourMaxFixedUpdatesPerFrame = 4;

    void RegisterCommands(const char* argv[]);

    CommonUtilities::InputHandler myInputHandler;
    CommonUtilities::Timer myTimer;

    CameraSystem& myCameraSystem;

    float myFixedTimeAccumulator = 0.0f;
    bool myIsFirstFrame;

    StateStack myWorldStateStack;

    unsigned int mySwitchState;
};
