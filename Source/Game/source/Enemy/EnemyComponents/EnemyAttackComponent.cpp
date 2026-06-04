#include "EnemyAttackComponent.h"
#include "EnemyMovementComponent.h"
#include "EnemyAnimationComponent.h"
#include "GameObject.h"
#include <iostream>

EnemyAttackComponent::EnemyAttackComponent(const EnemyData& someEnemyData)
{
	myEnemyData = someEnemyData;
}

void EnemyAttackComponent::OnStart()
{
	myAttackData.owner = GetOwner();
	myMovement = GetOwner()->GetComponent<EnemyMovementComponent>();
	myAnimation = GetOwner()->GetComponent<EnemyAnimationComponent>();

	switch (myEnemyData.EnemyType)
	{
	case EnemyType::BasicEnemy:
		myAttackData.team = CombatTeam::Enemy;
		myAttackData.type = AttackType::EnemyMelee;
		myAttackData.collisionShape = CollisionShapeType::Sphere;
		myAttackData.damage = 1;
		myAttackData.localCenterOffset = CommonUtilities::Vector3<float>(0.0f, 90.0f, 0.0f);
		myAttackData.radius = 190.0f;
		myAttackData.activeDurationSeconds = 0.16f;
		myAttackData.knockbackStrength = 450.0f;
		myAttackData.onlyHitForwardHemisphere = true;
		myAttackData.targetLayers.AddLayer(ObjectLayer::Player);
		break;
	case EnemyType::RollingEnemy:
		myAttackData.team = CombatTeam::Enemy;
		myAttackData.type = AttackType::EnemyRoll;
		myAttackData.collisionShape = CollisionShapeType::Sphere;
		myAttackData.damage = 1;
		myAttackData.localCenterOffset = CommonUtilities::Vector3<float>(0.0f, 90.0f, 0.0f);
		myAttackData.radius = 120.0f;
		myAttackData.activeDurationSeconds = 0.16f;
		myAttackData.knockbackStrength = 450.0f;
		myAttackData.onlyHitForwardHemisphere = true;
		myAttackData.isContinuous = true;
		myAttackData.targetLayers.AddLayer(ObjectLayer::Player);
		break;
	default:
		break;
	}

}

void EnemyAttackComponent::OnUpdate(float aDeltaTime)
{
	if (myCooldownTimer > 0.0f)
	{
		myCooldownTimer -= aDeltaTime;
	}

	switch (myState)
	{
	case AttackState::Idle:
		break;

	case AttackState::Windup:
		UpdateWindup(aDeltaTime);
		break;

	case AttackState::Active:
		UpdateActive(aDeltaTime);
		break;

	case AttackState::Recovery:
		UpdateRecovery(aDeltaTime);
		break;
	}
}

bool EnemyAttackComponent::CanAttack() const
{
	return myState == AttackState::Idle && myCooldownTimer <= 0.0f;
}

void EnemyAttackComponent::CancelAttack()
{
	myState = AttackState::Idle;
	myTimer = 0.0f;
	myHasAppliedAttack = false;
}

bool EnemyAttackComponent::DidRollHitPlayerThisFrame() const
{
	const auto& events = CombatService::Get()->GetHitEventsThisFrame();

	for (const auto& event : events)
	{
		if (event.attackId == myCombatAttackId &&
			event.target &&
			event.target->GetLayer() == ObjectLayer::Player)
		{
			std::cout << "Player hit detected by combat\n";
			return true;
		}
	}

	return false;
}

bool EnemyAttackComponent::DidRollHitWallThisFrame() const
{
	const auto& events = CombatService::Get()->GetHitEventsThisFrame();

	for (const auto& event : events)
	{
		if (event.attackId == myCombatAttackId &&
			event.target &&
			event.target->GetLayer() == ObjectLayer::WorldStatic)
		{
			return true;
		}
	}

	return false;
}

void EnemyAttackComponent::UpdateWindup(float aDeltaTime)
{
	myTimer -= aDeltaTime;

	if (myMovement)
	{
		myMovement->RotateTowards(myAttackDirection, aDeltaTime);
		myMovement->StopMoving();
		myRollStartPosition = GetOwner()->GetTransform().GetPosition();
	}

	if (myTimer <= 0.0f)
	{
		myState = AttackState::Active;
		myTimer = myEnemyData.AttackActiveDuration;
	}
}

void EnemyAttackComponent::UpdateActive(float aDeltaTime)
{
	if (!myHasAppliedAttack)
	{
		PerformAttack();
		myHasAppliedAttack = true;
		myCooldownTimer = myEnemyData.AttackCooldown;
	}

	if (myEnemyData.EnemyType == EnemyType::RollingEnemy && myMovement)
	{
		if (!CombatService::IsAttackActive(myCombatAttackId))
		{
			FinishRollingAttack();
			return;
		}

		if (myDidCollideWithWall)
		{
			myDidCollideWithWall = false;

			FinishRollingAttack();
			return;
		}

		myMovement->MoveForward(aDeltaTime);
		return;
	}

	myTimer -= aDeltaTime;

	if (myTimer <= 0.0f)
	{
		myState = AttackState::Recovery;

		myTimer = myEnemyData.AttackRecovery;
	}

}

void EnemyAttackComponent::UpdateRecovery(float aDeltaTime)
{
	myTimer -= aDeltaTime;

	if (myTimer <= 0.0f)
	{
		myState = AttackState::Idle;
	}
}

void EnemyAttackComponent::PerformAttack()
{
	auto& transform = GetOwner()->GetTransform();

	float angle = std::atan2(myAttackDirection.x, myAttackDirection.z);
	auto rot = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(Vector3f::UnitY, angle);

	transform.SetRotation(rot);

	myCombatAttackId = CombatService::StartAttack(myAttackData);
}

void EnemyAttackComponent::FinishRollingAttack()
{
	if (myCombatAttackId != 0)
	{
		CombatService::CancelAttack(myCombatAttackId);
	}

	myFinishedRollingAttack = true;
	myState = AttackState::Recovery;
	myTimer = myEnemyData.AttackRecovery;
	myCooldownTimer = myEnemyData.AttackCooldown;

	if (myMovement)
	{
		myMovement->StopMoving();
		myMovement->SetMovementSpeed(0.f);
	}

	myCombatAttackId = 0;
}

void EnemyAttackComponent::StopRollingAttackOnWorldCollision(GameObject& anOther)
{

	if (myEnemyData.EnemyType != EnemyType::RollingEnemy)
	{
		return;
	}

	if (myState != AttackState::Active)
	{
		return;
	}

	if (anOther.GetLayer() != ObjectLayer::WorldStatic)
	{
		return;
	}

	std::cout << "WORLD COLLISION DETECTED\n";

	myDidCollideWithWall = true;
}

bool EnemyAttackComponent::ConsumeWallHit()
{
	if (!myDidCollideWithWall)
		return false;

	myDidCollideWithWall = false;
	return true;
}

bool EnemyAttackComponent::IsRollingActive() const
{
	return myState == AttackState::Active && myEnemyData.EnemyType == EnemyType::RollingEnemy;
}

void EnemyAttackComponent::OnCollisionEnter(const CollisionContact& /*aContact*/, GameObject& anOther)
{
	if (anOther.GetLayer() == ObjectLayer::WorldStatic)
	{
		StopRollingAttackOnWorldCollision(anOther);
	}

}

void EnemyAttackComponent::OnCollisionStay(const CollisionContact& /*aContact*/, GameObject& anOther)
{
	if (anOther.GetLayer() == ObjectLayer::WorldStatic)
	{
		StopRollingAttackOnWorldCollision(anOther);
	}
}

void EnemyAttackComponent::OnCollisionExit(const CollisionContact& /*aContact*/, GameObject& /*anOther*/)
{
}

bool EnemyAttackComponent::IsAttacking() const
{
	return myState != AttackState::Idle;
}

void EnemyAttackComponent::StartAttack(GameObject* aTarget)
{
	if (!CanAttack() || !aTarget)
	{
		return;
	}

	auto& ownerPos = GetOwner()->GetTransform().GetPosition();
	auto& targetPos = aTarget->GetTransform().GetPosition();

	myAttackDirection = (targetPos - ownerPos);
	myAttackDirection.y = 0;
	myAttackDirection.Normalize();

	myState = AttackState::Windup;
	myTimer = myEnemyData.AttackWindup;

	myHasAppliedAttack = false;
	myFinishedRollingAttack = false;
	myDidCollideWithWall = false;
}
