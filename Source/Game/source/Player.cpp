#include "Player.h"
#include "AnimatedMeshComponent.h"
#include "Essentials/Essentials.h"

Player::Player(std::string aName) 
	: GameObject(std::move(aName))
{
}
