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
	int Health;
	int Damage;
	float AttackRange;
	int AggroRange;
	float MoveSpeed;
};