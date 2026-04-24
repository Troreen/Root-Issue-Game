#include "Particle.h"

void Particle::Init(const ParticleSettings& someParticleSettings)
{
	mySettings = someParticleSettings;
	myTransform(4, 1) = someParticleSettings.initalPosition.x;
	myTransform(4, 2) = someParticleSettings.initalPosition.y;
	myTransform(4, 3) = someParticleSettings.initalPosition.z;
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

	myTransform(4, 1) += mySettings.linearVelocity.x;
	myTransform(4, 2) += mySettings.linearVelocity.y;
	myTransform(4, 3) += mySettings.linearVelocity.z;
	myInstance.myTransform = myTransform;
	
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
