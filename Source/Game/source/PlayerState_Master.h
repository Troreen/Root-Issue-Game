#pragma once
#include "PlayerState_Walk.h"
#include "GameObject.h"
#include <memory>

class PlayerState_Master
{
public:
	PlayerState_Master();
	~PlayerState_Master() = default;

	static PlayerState_Master& Instance();

	static inline std::unique_ptr<PlayerState_Walk> myWalkState;
private:
};

