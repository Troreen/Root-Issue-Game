#pragma once
#include "State.hpp"
#include "CombatSystem.h"
#include "RuntimeCollisionSystem.h"

class InGame : public State
{
public:
	InGame() = default;

	void Init(CameraSystem& aCamera, const char* argv[]) override;
	eState Update() override;
	void Render() override;

private:
	void ConsumeCollisionContacts(const std::vector<CollisionContact>& someContacts);

	CombatSystem myCombatSystem;
	RuntimeCollisionSystem myRuntimeCollisionSystem;
};
