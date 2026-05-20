#pragma once

enum class EnemyType
{
    BasicEnemy,
    RollingEnemy,
    Unknown
};

struct EnemyData
{
    EnemyType EnemyType = EnemyType::Unknown;

    bool ShouldSpawn = false;
    float SpawnTime = 2.0f;

    // Movement
    float WalkSpeed = 300.0f;
    float ChaseSpeed = 450.0f;
    float RotationSpeed = 5.0f;

    // Targeting
    float DetectionRange = 600.0f;

    // AI

    float IdleTimeMin = 1.0f;
    float IdleTimeMax = 2.0f;

    float WanderTimeMin = 1.5f;
    float WanderTimeMax = 3.0f;

    float WanderTurnAngleMin = -30.0f;
    float WanderTurnAngleMax = 30.0f;

    // Attack
    float AttackRange = 200.0f;
    float AttackCooldown = 2.0f;
    float AttackWindup = 0.3f;
    float AttackRecovery = 1.0f;

    // Rolling enemy
    float RollSpeed = 800.0f;
    float RollDuration = 1.0f;
    float StunTime = 1.5f;
    float RollDistance = 600.0f;
};