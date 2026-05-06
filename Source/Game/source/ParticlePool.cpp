#include "ParticlePool.h"

#include <tge/drawers/SpriteDrawer.h>
#include <tge/engine.h>
#include <tge/texture/TextureManager.h>
#include <tge/graphics/GraphicsEngine.h>
#include "tge/graphics/GraphicsStateStack.h"


ParticlePool::ParticlePool()
{
	myPoolSize = 0;
	myNbrOfActiveParticles = 0;
	myFirstAvailable = nullptr;
}

void ParticlePool::Init(size_t aPoolSize, const Tga::TextureResource& aTexture, const Tga::BlendState& aBlendState)
{
	myBlendState = aBlendState;

	mySpriteData.myTexture = &aTexture;

	myPoolSize = aPoolSize;
	myParticles.resize(myPoolSize);
	myInstances.resize(myPoolSize);

	myFirstAvailable = &myParticles[0];

	Tga::Vector2ui size = mySpriteData.myTexture->CalculateTextureSize();

	for (int i = 0; i < myPoolSize - 1; ++i) 
	{
		myParticles[i].SetNext(&myParticles[i + 1]);
		myParticles[i].SetSize(size);
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
	Tga::Engine& engine = *Tga::Engine::GetInstance();
	engine.GetGraphicsEngine().GetGraphicsStateStack().Push();

	engine.GetGraphicsEngine().GetGraphicsStateStack().SetBlendState(myBlendState);
	Tga::SpriteBatchScope batch = Tga::Engine::GetInstance()->GetGraphicsEngine().GetSpriteDrawer().BeginBatch(mySpriteData);
	batch.Draw(myInstances.data(), myNbrOfActiveParticles);

	engine.GetGraphicsEngine().GetGraphicsStateStack().Pop();

	//std::cout << "nbr of active particles to render: " << myNbrOfActiveParticles << std::endl;

}

void ParticlePool::Create(const ParticleSettings& someParticleSettings)
{

	//std::cout << "Create called" << std::endl;

	if (myFirstAvailable != nullptr)
	{
		Particle* newParticle = myFirstAvailable;
		myFirstAvailable = newParticle->GetNext();

		newParticle->Init(someParticleSettings);
	}
	
	else
	{
		//std::cout << "Particle pool exhausted\n";
	}
}
