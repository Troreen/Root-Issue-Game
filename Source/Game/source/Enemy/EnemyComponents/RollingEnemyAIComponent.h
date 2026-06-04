#pragma once
#include "EnemyAIComponent.h"

class RollingEnemyAIComponent : public EnemyAIComponent
{
public:

    RollingEnemyAIComponent(const EnemyData& someEnemyData);

    void OnUpdate(float aDeltaTime) override;

private:

    void StopRollingSound();
    void UpdateSpawn(float aDeltaTime);
    void UpdateIdle(float aDeltaTime);
    void UpdateWander(float aDeltaTime);
    void UpdateReact(float aDeltaTime);
    void UpdateAttacking(float aDeltaTime);
    void UpdateHurt(float aDeltaTime);
    void UpdateStunned(float aDeltaTime);
    void UpdateDeath(float aDeltaTime);
    void UpdateReturnHome(float aDeltaTime);

    void EnterStunnedState();

    bool myIsRollingAnimActive = false;
};