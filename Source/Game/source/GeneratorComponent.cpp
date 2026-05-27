#include "GeneratorComponent.h"
#include "ParticleEmitterComponent.h"

#include <random>

void GeneratorComponent::OnUpdate(float aDeltaTime)
{
	myTimer += aDeltaTime;
	if (myTimer >= myEmissionInterval)
	{
		ParticleEmitterComponent* emitter = GetOwner()->GetComponent<ParticleEmitterComponent>();
		emitter->Burst(ParticleType::Spark); 

		std::mt19937 rng(std::random_device{}());
		std::uniform_real_distribution<float> dist(0.5f, 2.5f);
		myEmissionInterval = dist(rng);

		myTimer = 0;
	}
}
