#pragma once
#include "EnemyAIComponent.h"

class MeleeEnemyAIComponent : public EnemyAIComponent
{
public:

    MeleeEnemyAIComponent(const EnemyData& someEnemyData);

    void OnUpdate(float aDeltaTime) override;

private:

    void UpdateSpawn(float aDeltaTime);
    void UpdateIdle(float aDeltaTime);
    void UpdateWander(float aDeltaTime);
    void UpdateChasing(float aDeltaTime);
    void UpdateAttacking(float aDeltaTime);
    void UpdateHurt(float aDeltaTime);
    void UpdateDeath(float aDeltaTime);
    void UpdateReturnHome(float aDeltaTime);

    bool CheckIfDeadOrTookDamage();
};