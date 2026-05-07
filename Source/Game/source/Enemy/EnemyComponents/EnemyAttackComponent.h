#pragma once
#include "ScriptComponent.h"
#include "CombatSystem.h"

class EnemyAttackComponent : public ScriptComponent
{
public:

	EnemyAttackComponent();



private:

	AttackData myAttackData;

};

