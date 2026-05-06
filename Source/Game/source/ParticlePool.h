#pragma once

#include "Particle.h"
#include <vector>

#include <tge/sprite/sprite.h>

class ParticlePool
{
public:
	ParticlePool();
	~ParticlePool() = default;

	void Init(size_t aPoolSize);
	void Update(float aTimeDelta);
	void Render() const;

	void Create(const ParticleSettings& someParticleSettings);

private:
	
	size_t myPoolSize;
	size_t myNbrOfActiveParticles;
	std::vector<Particle> myParticles;
	std::vector<Tga::Sprite3DInstanceData> myInstances;

	Particle* myFirstAvailable;

	Tga::SpriteSharedData mySpriteData;
};

