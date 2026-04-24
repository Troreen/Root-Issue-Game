#pragma once

enum class EnemyType
{
	BasicEnemy,
	RollingEnemy,
	Unknown
};

struct EnemyData
{
	int Health;
	int Damage;
	float AttackRange;
	int AggroRange;
	float MoveSpeed;
};