#include "ParticlePool.h"

#include <tge/drawers/SpriteDrawer.h>
#include <tge/engine.h>
#include <tge/graphics/GraphicsEngine.h>

ParticlePool::ParticlePool()
{
	myPoolSize = 0;
	myNbrOfActiveParticles = 0;
	myFirstAvailable = nullptr;
}

void ParticlePool::Init(size_t aPoolSize)
{
	myPoolSize = aPoolSize;
	myParticles.resize(myPoolSize);
	myInstances.resize(myPoolSize);

	myFirstAvailable = &myParticles[0];

	for (int i = 0; i < myPoolSize - 1; ++i) 
	{
		myParticles[i].SetNext(&myParticles[i + 1]);
	}

	myParticles[myPoolSize - 1].SetNext(nullptr);
}

void ParticlePool::Update(float aTimeDelta)
{
	myNbrOfActiveParticles = 0;

	for (size_t i = 0; i < myPoolSize; ++i)
	{
		if (!myParticles[i].InUse())
		{
			continue;
		}
		if (myParticles[i].Animate(aTimeDelta))
		{
			myParticles[i].SetNext(myFirstAvailable);
			myFirstAvailable = &myParticles[i];
		}
		else
		{
			myInstances[myNbrOfActiveParticles++] = myParticles[i].GetInstance();
		}
	}
}

void ParticlePool::Render() const
{
	Tga::SpriteBatchScope batch = Tga::Engine::GetInstance()->GetGraphicsEngine().GetSpriteDrawer().BeginBatch(mySpriteData);

	batch.Draw(myInstances.data(), myNbrOfActiveParticles);
}

void ParticlePool::Create(const ParticleSettings& someParticleSettings)
{
	if (myFirstAvailable != nullptr)
	{
		Particle* newParticle = myFirstAvailable;
		myFirstAvailable = newParticle->GetNext();

		newParticle->Init(someParticleSettings);
	}
}
