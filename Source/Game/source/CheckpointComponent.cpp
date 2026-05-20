#include "CheckpointComponent.h"
#include "GameObject.h"
#include "DamageableComponent.h"
#include "ParticleEmitterComponent.h"
#include "Essentials/Essentials.h"

void CheckpointComponent::Toggle()
{
	if (myIsActive) return;
	myIsActive = true;

	ParticleEmitterComponent* emitter = GetOwner()->GetComponent<ParticleEmitterComponent>();
	emitter->SetEmissionDirection(ParticleType::Energy, { 0,-1,0 });
	emitter->SetEmissionWithDuration(ParticleType::Energy, 2.f);

	Essentials::globalPostMaster->SendMsg({ MessageType::SaveScene });
}
