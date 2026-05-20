#pragma once
#include "ScriptComponent.h"
#include "CommonUtilities/Vector.hpp"
#include <vector>
#include <memory>
#include "GameObject.h"
#include "CapsuleColliderComponent.h"
#include "CombatSystem.h"


class BulletComponent : public ScriptComponent
{
public:
	BulletComponent();
	void SetTransform(CommonUtilities::Transform<float> aTransform);
	void OnUpdate(float aDeltaTime) override;

private:

	float myTimer;
	float mySpeed;

	CommonUtilities::Transform<float> myTransform;
	CapsuleColliderComponent* myCollider;
	AttackData myAttackData;
};

