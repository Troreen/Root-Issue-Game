#pragma once

// Shared game flow vocabulary used by menu, scene, and world systems.
enum class GameFlowMode
{
    Title,
    Credits,
    LevelSelect,
    Loading,
    Gameplay,
    Paused,
    LevelResults
};

inline const char* ToString(const GameFlowMode aMode)
{
    switch (aMode)
    {
    case GameFlowMode::Title:
        return "Title";
    case GameFlowMode::Credits:
        return "Credits";
    case GameFlowMode::LevelSelect:
        return "LevelSelect";
    case GameFlowMode::Loading:
        return "Loading";
    case GameFlowMode::Gameplay:
        return "Gameplay";
    case GameFlowMode::Paused:
        return "Paused";
    case GameFlowMode::LevelResults:
        return "LevelResults";
    default:
        return "Unknown";
    }
}
