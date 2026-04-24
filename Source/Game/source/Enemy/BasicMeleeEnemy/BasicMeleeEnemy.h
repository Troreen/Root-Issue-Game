#pragma once
#include "GameObject.h"

class BasicMeleeEnemy : public GameObject
{
public:
	explicit BasicMeleeEnemy(std::string aName = "BasicMeleeEnemy");
	~BasicMeleeEnemy() override = default;
};

