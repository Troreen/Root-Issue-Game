#include "BulletComponent.h"
#include "CommonUtilities/Transform.hpp"
#include "GameObject.h"
void BulletComponent::OnUpdate(float aDeltaTime)
{
    	if (!GetOwner()->IsActive()) return;

	if (myLifeTimer < 0)
	{
		GetOwner()->SetActive(false);
	}

	myLifeTimer -= aDeltaTime;

	CommonUtilities::Transform<float>& transform = GetOwner()->GetTransform();

	transform.Translate(myDirection * mySpeed);
}

void BulletComponent::SetSpeedDirectionPosition(float aSpeed, CommonUtilities::Vector3<float> aDirection, CommonUtilities::Vector3<float>  aPos)
{
	GetOwner()->SetActive(true);
	myLifeTimer = 2.f;
	mySpeed = aSpeed;
	myDirection = aDirection;

	GetOwner()->GetTransform().SetPosition(aPos);
}

