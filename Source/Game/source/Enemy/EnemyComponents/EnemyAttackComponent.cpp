#include "EnemyAttackComponent.h"
#include "GameObject.h"

EnemyAttackComponent::EnemyAttackComponent()
{
	myAttackData.owner = GetOwner();
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
}
