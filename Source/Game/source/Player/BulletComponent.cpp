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

void BulletComponent::SetTransform(CommonUtilities::Transform<float> aTransform)
{
	mySpeed = 1000.f;
	myTimer = 2.f;
	myTransform = aTransform;
	GetOwner()->GetTransform() = aTransform;
}

void BulletComponent::OnUpdate(float aDeltaTime)
{
	std::cout << "Cock\n";
	myTimer -= aDeltaTime;
	if (myTimer < 0)
	{
		GetOwner()->SetActive(false);
	}

	GetOwner()->GetTransform().Translate(myTransform.GetForward() * mySpeed * aDeltaTime);
}

