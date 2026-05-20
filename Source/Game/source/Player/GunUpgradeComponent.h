#pragma once
#include "ScriptComponent.h"
#include "CapsuleColliderComponent.h"

struct SceneObjectData;

class GunUpgradeComponent : public ScriptComponent
{
public:

	void OnStart() override;
	void OnUpdate(float) override;

private:

	bool myEnable;
	CapsuleColliderComponent* myCollider;
};