#pragma once
#include "PlayerState.h"
#include "GameObject.h"

class Player : public GameObject
{
public:
	explicit Player(std::string aName = "Player");
	~Player() = default;

private:
	PlayerState* myPlayerState;

};

