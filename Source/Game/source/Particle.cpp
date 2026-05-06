#include "Particle.h"

void Particle::Init(const ParticleSettings& someParticleSettings)
{
	mySettings = someParticleSettings;
	myTransform.SetPosition(someParticleSettings.initalPosition);
	myTimeLeft = mySettings.timeToLive;
}

void Particle::SetNext(Particle* aNext) 
{
	myNext = aNext;
}

Particle* Particle::GetNext() const
{
	return myNext;
}

bool Particle::Animate(float aTimeDelta)
{

	if (!InUse()) 
	{
		return false;
	}

	myTransform.Translate(mySettings.linearVelocity * aTimeDelta);

	Tga::Vector3f pos = myTransform.GetPosition().ToTga();

	myInstance.myTransform.SetPosition(pos);

	myTimeLeft -= aTimeDelta;
	return myTimeLeft <= 0;
}

const Tga::Sprite3DInstanceData& Particle::GetInstance() const
{
	return myInstance;
}

bool Particle::InUse() const
{
	return myTimeLeft > 0;
}
