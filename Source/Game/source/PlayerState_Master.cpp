#include "PlayerState_Master.h"
#include "Essentials/Essentials.h"


PlayerState_Master::PlayerState_Master()
{
	myWalkState = std::make_unique<PlayerState_Walk>();
}

PlayerState_Master& PlayerState_Master::Instance()
{
	static PlayerState_Master instance;
	return instance;
}
