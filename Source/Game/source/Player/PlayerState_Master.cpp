#include "PlayerState_Master.h"
#include "Essentials/Essentials.h"


PlayerState_Master::PlayerState_Master()
{
	myWalkState = std::make_unique<PlayerState_Walk>();
	myAttackState = std::make_unique<PlayerState_Attack>();
	myChargeAttackState = std::make_unique<PlayerState_Charge_Attack>();
	myShootState = std::make_unique<PlayerState_Shoot>();
	myDeathState = std::make_unique<PlayerState_Death>();
	myUpgradeState = std::make_unique<PlayerState_Pickup_Gun>();
	myHurtState = std::make_unique<PlayerState_Hurt>();
}

PlayerState_Master& PlayerState_Master::Instance()
{
	static PlayerState_Master instance;
	return instance;
}
