#include "BulletComponent.h"
#include "CommonUtilities/Transform.hpp"
#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/ModelDrawer.h>
#include <tge/model/ModelInstance.h>
#include <tge/model/ModelFactory.h>
#include <tge/engine.h>
#include "GameObject.h"
#include "Essentials/Essentials.h"


namespace
{
	Tga::Matrix4x4f ToTgaMatrix(const CommonUtilities::Matrix4x4<float>& aMatrix)
	{
		Tga::Matrix4x4f result;
		for (int r = 1; r < 5; ++r)
		{
			for (int c = 1; c < 5; ++c)
			{
				result(r, c) = aMatrix(r, c);
			}
		}
		return result;
	}

}

BulletComponent::BulletComponent()
{

}

void BulletComponent::SetTransform(CommonUtilities::Transform<float> aTransform)
{
	myAttackData.owner = GetOwner();
	myAttackData.team = CombatTeam::Player;
	myAttackData.type = AttackType::Ranged;
	myAttackData.collisionShape = CollisionShapeType::Sphere;
	myAttackData.damage = 1;
	myAttackData.localCenterOffset = CommonUtilities::Vector3<float>(0.0f, 100.f, 0.0f);
	myAttackData.radius = 50.0f;
	myAttackData.activeDurationSeconds = 0.1f;
	myAttackData.knockbackStrength = 450.0f;
	myAttackData.onlyHitForwardHemisphere = false;
	myAttackData.targetLayers.AddLayer(ObjectLayer::Enemy);
	myAttackData.targetLayers.AddLayer(ObjectLayer::Switch);

	myCollider = GetOwner()->GetComponent<CapsuleColliderComponent>();
	mySpeed = 2000.f;
	myTimer = 2.f;
	myTransform = aTransform;
	GetOwner()->GetTransform() = aTransform;
}

void BulletComponent::OnUpdate(float aDeltaTime)
{
	myTimer -= aDeltaTime;
	if (myTimer < 0)
	{
		GetOwner()->SetActive(false);
	}

	if (myCollider->IsInside())
	{
		std::cout << "Inside\n";

		CombatService::StartAttack(myAttackData);

		
		/*(*this).SetEnabled(false);*/
		GetOwner()->DisableAllComponents();
	}
	GetOwner()->GetTransform().Translate(myTransform.GetForward() * mySpeed * aDeltaTime);
}

