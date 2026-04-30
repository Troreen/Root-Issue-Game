#pragma once
#include "PlayerState_Walk.h"
#include "PlayerState_Attack.h"
#include "PlayerState_Charge_Attack.h"
#include "PlayerState_Shoot.h"
#include "GameObject.h"
#include <memory>

class PlayerState_Master
{
public:
	PlayerState_Master();
	~PlayerState_Master() = default;

	static PlayerState_Master& Instance();

	static inline std::unique_ptr<PlayerState_Walk> myWalkState;
	static inline std::unique_ptr<PlayerState_Attack> myAttackState;
	static inline std::unique_ptr<PlayerState_Charge_Attack> myChargeAttackState;
	static inline std::unique_ptr<PlayerState_Shoot> myShootState;
private:
};

