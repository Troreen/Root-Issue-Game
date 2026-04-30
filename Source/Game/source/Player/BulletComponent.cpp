#include "BulletComponent.h"
#include "CommonUtilities/Transform.hpp"
#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/ModelDrawer.h>
#include <tge/model/ModelInstance.h>
#include <tge/model/ModelFactory.h>
#include <tge/engine.h>
#include "GameObject.h"

BulletComponent::BulletComponent()
{
	Tga::ModelFactory& modelFactory = Tga::ModelFactory::GetInstance();

	Tga::ModelInstance instance = modelFactory.GetModelInstance("animations/SK/SK_CH_Player.fbx");

	myBullet = Bullet(instance);
}

void BulletComponent::OnUpdate(float aDeltaTime)
{
	for (int bulletIndex = 0; bulletIndex < myBullets.size(); bulletIndex++)
	{
		myBullets[bulletIndex].Update(aDeltaTime);

		if (myBullets[bulletIndex].IsDelete())
		{
			myBullets[bulletIndex] = myBullets.back();
			myBullets.pop_back();
			if (bulletIndex < myBullets.size() && !myBullets.empty())
			{
				bulletIndex--;
			}
		}
	}
}

void BulletComponent::Render()
{
	auto& graphicsEngine = Tga::Engine::GetInstance()->GetGraphicsEngine();
	auto& modelDrawer = graphicsEngine.GetModelDrawer();

	for (auto& bullet : myBullets)
	{
		bullet.Render(modelDrawer);
	}
}

void BulletComponent::SetSpeedDirectionPosition(float aSpeed, CommonUtilities::Vector3<float> aDirection)
{
	aSpeed;
	aDirection;
}

void BulletComponent::SpawnBullet()
{
	myBullet.Init(GetOwner()->GetTransform());
	myBullets.push_back(myBullet);
}

