#pragma once

#include <CommonUtilities/Vector3.hpp>

class GameObject;

enum class CollisionPhase
{
    Enter,
    Stay,
    Exit
};

enum class CollisionRule
{
    Ignore,
    Block,
    Trigger
};

struct CollisionContact
{
    GameObject* first = nullptr;
    GameObject* second = nullptr;
    CommonUtilities::Vector3<float> normal = { 0.0f, 0.0f, 0.0f };
    float penetration = 0.0f;
    CollisionPhase phase = CollisionPhase::Stay;
};
