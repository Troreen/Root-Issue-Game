#pragma once
#include "ScriptComponent.h"
#include "EnemyData.h"
#include <string>
#include <Vector.hpp>
#include <random>
#include "Subscriber.h"

class EnemyMovementComponent;
class EnemyTargetingComponent;
class EnemyAnimationComponent;
class ParticleEmitterComponent;
class EnemyAttackComponent;
class DamageableComponent;
class SphereColliderComponent;

enum class EnemyState
{
	Spawn,
	Idle,
	Wander,
	Chasing,
	Attacking,
	Hurt,
	Stunned,
	Death,
	COUNT
};

class EnemyAIComponent : public ScriptComponent
{
public:

	EnemyAIComponent() = delete;
	EnemyAIComponent(const EnemyData& someEnemyData);
	~EnemyAIComponent();

	void Init(Tga::Engine& aEngine) override;
	void OnUpdate(float aDeltaTime) override;

	void SetAggro(bool aState);

	void Reset() override;
	void Save() override;
	void Spawn();
private:

	//Basic Enemy
	void HandleStatesBasicEnemy(float aDeltaTime);
	void HandleStatesRollingEnemy(float aDeltaTime);

	void ChangeState(const EnemyState& aState);

	std::string StringifyState(const EnemyState& aState) const;

	void UpdateSpawn(float aDeltaTime);
	void UpdateIdle(float aDeltaTime);
	void UpdateWander(float aDeltaTime);
	void UpdateChasing(float aDeltaTime);
	void UpdateAttacking(float aDeltaTime);
	void UpdateHurt(float aDeltaTime);
	void UpdateStunned(float aDeltaTime);
	void UpdateDeath(float aDeltaTime);

	void PickNewDirection();
	void ResetAnimations();

	float GetRandomAngleDegreeToRad(float aMin, float aMax);
	float GetRandomFloat(float aMin, float aMax);


	void BasicEnemyLogicUpdate(float aDeltaTime);
	void RollingEnemyLogicUpdate(float aDeltaTime);

	void AILogicUpdate(float aDeltaTime);

	EnemyType myType;

	EnemyData myEnemyData;

	EnemyState myCurrentState;
	EnemyState myPreviousState;

	//RollingEnemyState myCurrentStateForRolling;
	//RollingEnemyState myPreviousStateForRolling;

	EnemyMovementComponent* myMovement = nullptr;
	EnemyTargetingComponent* myTargeting = nullptr;
	EnemyAttackComponent* myAttack = nullptr;

	EnemyAnimationComponent* myAnimation = nullptr;
	ParticleEmitterComponent* myEmitterComponent = nullptr;
	DamageableComponent* myDamageableComponent = nullptr;

	SphereColliderComponent* myCollider = nullptr;


	std::mt19937 myRandomEngine;

	bool myIsAggro = false;
	bool myHasBeenInitialized = false;
	bool myActiveAfterSave = true;

	float mySpawnTimer = 0.0f;
	float myIdleTimer = 0.0f;
	float myWanderTimer = 0.0f;
	float myDeathTimer = 3.0f;
	float myHurtTimer = 0.5f;
	float myStunnedTimer = 0.5f;
	const float myHurtDuration = 0.5f;
	float myMaxSpeed = 400.0f;

	CommonUtilities::Vector3<float> myWanderDirection;

};

