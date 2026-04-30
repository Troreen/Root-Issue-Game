/*
Example Usage of Command Registration Helpers:

#include "Console.h"
#include <string>
#include <vector>
#include <functional>
#include <numeric>

enum class LogLevel { Info, Warning, Error, Command, Usage, Success };

// ----------------------
// Integer Command
// ----------------------
// FOV between 1-160
RegisterIntCommand(
    "Graphics",                // category
    "fov",                     // name
    "fov <value>",             // usage
    "Set the camera's field of view", // description
    [](int value) { myCamera->SetFOV(value); },
    1, 160                     // min/max
);

// ----------------------
// Float Command
// ----------------------
// Mouse sensitivity between 0.1 and 10.0
RegisterFloatCommand(
    "Gameplay",                // category
    "mouseSens",               // name
    "mouseSens <value>",       // usage
    "Set mouse sensitivity",   // description
    [](float value) { mouseSensitivity = value; },
    0.1f, 10.f                 // min/max
);

// ----------------------
// Boolean Command
// ----------------------
// Toggle debug mode on/off
RegisterBoolCommand(
    "Debug",                   // category
    "debug",                   // name
    "debug <true/false>",      // usage
    "Enable or disable debug mode", // description
    [](bool value) { enableDebug = value; }
);

// ----------------------
// Enum Command
// ----------------------
// Rendering mode: wireframe, solid, or textured
RegisterEnumCommand(
    "Graphics",                                // category
    "renderMode",                              // name
    "renderMode <wireframe|solid|textured>",   // usage
    "Change the rendering mode",               // description
    [](const std::string& mode)
    {
        if (mode == "wireframe") { ... }
        else if (mode == "solid") { ... }
        else if (mode == "textured") { ... }
    },
    { "wireframe", "solid", "textured" }      // allowed values
);

// ----------------------
// Multi-Argument Command
// ----------------------
// Spawn enemies: type, count, optional flags
RegisterMultiArgCommand(
    "Gameplay",                                // category
    "spawnEnemies",                             // name
    "spawnEnemies <type> <count> [extra params]", // usage
    "Spawn multiple enemies of a given type with optional extra parameters", // description
    [](const std::vector<std::string>& args)
    {
        if (args.size() < 2)
        {
            LOG("Usage: spawnEnemies <type> <count> [extra params]", LogLevel::Usage);
            return;
        }

        std::string enemyType = args[0];
        int count = std::stoi(args[1]);
        std::vector<std::string> extras(args.begin() + 2, args.end());

        LOG("Spawning " + std::to_string(count) + " " + enemyType + "(s)", LogLevel::Info);

        if (!extras.empty())
        {
            std::string extraInfo = std::accumulate(extras.begin(), extras.end(), std::string(),
                [](std::string a, std::string b) { return a.empty() ? b : a + ", " + b; });
            LOG("Extra parameters: " + extraInfo, LogLevel::Info);
        }

        // Call game logic to spawn the enemies here
    }
);

// ----------------------
// Raw Command
// ----------------------
// Access arguments directly
RegisterCommand(
    "General",                  // category
    "echo",                     // name
    "echo <text>",              // usage
    "Prints a message to the console", // description
    [](const std::vector<std::string>& args)
    {
        std::string msg;
        for (auto& s : args) msg += s + " ";
        LOG(msg, LogLevel::Command);
    }
);

// ----------------------
// Using CMD Macro
// ----------------------
CMD("General", "help", "help", "Displays all available commands",
    [](const std::vector<std::string>& args)
    {
        Console::Get().ListCommands();
    });
*/