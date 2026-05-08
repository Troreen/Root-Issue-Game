#pragma once
#include "ScriptComponent.h"
#include "EnemyData.h"
#include <string>
#include <Vector.hpp>
#include <random>

class EnemyMovementComponent;
class EnemyTargetingComponent;
class AnimatedMeshComponent;
class AnimationGraphComponent;
class ParticleEmitterComponent;
class EnemyAttackComponent;
class DamageableComponent;

enum class BasicEnemyState
{
	Idle,
	Wander,
	Chasing,
	Attacking,
	Hurt,
	Death,
	COUNT
};

//enum class RollingEnemyState
//{
//	Idle,
//	Wander,
//	Chasing,
//	Attacking,
//  Stunned,
//	Death
//};

class EnemyAIComponent : public ScriptComponent
{
public:

	EnemyAIComponent() = delete;
	EnemyAIComponent(const EnemyData& someEnemyData);
	~EnemyAIComponent();

	void Init(Tga::Engine& aEngine) override;
	void OnStart() override;
	void OnUpdate(float aDeltaTime) override;

	void SetAggro(bool aState);

	void Reset() override;

private:

	//Basic Enemy
	void HandleStatesBasicEnemy(float aDeltaTime);

	void ChangeState(const BasicEnemyState& aState);

	std::string StringifyState(const BasicEnemyState& aState) const;

	void UpdateIdle(float aDeltaTime);
	void UpdateWander(float aDeltaTime);
	void UpdateChasing(float aDeltaTime);
	void UpdateAttacking(float aDeltaTime);
	void UpdateHurt(float aDeltaTime);
	void UpdateDeath(float aDeltaTime);

	void PickNewDirection();
	void ResetAnimations();

	float GetRandomAngleDegreeToRad(float aMin, float aMax);
	float GetRandomFloat(float aMin, float aMax);

	void HandleStatesRollingEnemy();

	void BasicEnemyLogicUpdate(float aDeltaTime);
	void RollingEnemyLogicUpdate(float aDeltaTime);


	void AILogicUpdate(float aDeltaTime);

	EnemyType myType;

	EnemyData myEnemyData;

	BasicEnemyState myCurrentState;
	BasicEnemyState myPreviousState;

	//RollingEnemyState myCurrentStateForRolling;
	//RollingEnemyState myPreviousStateForRolling;

	EnemyMovementComponent* myMovement = nullptr;
	EnemyTargetingComponent* myTargeting = nullptr;
	EnemyAttackComponent* myAttack = nullptr;

	AnimatedMeshComponent* myAnimation = nullptr;
	AnimationGraphComponent* myAnimationGraph = nullptr;
	ParticleEmitterComponent* myEmitterComponent = nullptr;
	DamageableComponent* myDamageableComponent = nullptr;


	std::mt19937 myRandomEngine;

	bool myIsAggro = false;
	bool myHasBeenInitialized = false;

	float myAnimationWeight;
	//float myChangeFromIdleTime = 5.0f;
	float myIdleTimer = 0.0f;
	float myWanderTimer = 0.0f;
	float myDeathTimer = 3.0f;
	float myHurtTimer = 0.5f;
	float myMaxSpeed = 400.0f;

	CommonUtilities::Vector3<float> myWanderDirection;

};

