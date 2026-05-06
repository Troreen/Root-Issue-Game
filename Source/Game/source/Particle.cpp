#include "Particle.h"
#include "Essentials/Essentials.h"
#include <cassert>

void Particle::Init(const ParticleSettings& someParticleSettings)
{
	mySettings = someParticleSettings;
	myTransform.SetPosition(someParticleSettings.initalPosition);
	myTimeLeft = mySettings.timeToLive;

	// TODO: Multiply in order Scale * Rotation * Translation to make the new transform. 
	// Maybe do it in the emitter instead??? Make sure it doesn't cost too much doing these multiplications!!!

	myInstance.myTransform.SetForward(mySettings.rotation.GetForward().ToTga());
	myInstance.myTransform.SetRight(mySettings.rotation.GetRight().ToTga());
	myInstance.myTransform.SetUp(mySettings.rotation.GetUp().ToTga());

	assert(mySize.LengthSqr() > 0);

	myInstance.myTransform(1, 1) *= static_cast<float>(mySize.x);
	myInstance.myTransform(1, 2) *= static_cast<float>(mySize.x);
	myInstance.myTransform(1, 3) *= static_cast<float>(mySize.x);
	myInstance.myTransform(2, 1) *= static_cast<float>(mySize.y);
	myInstance.myTransform(2, 2) *= static_cast<float>(mySize.y);
	myInstance.myTransform(2, 3) *= static_cast<float>(mySize.y);

	// Temp
	mySettings.swayAmplitude = 0;
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

	// TODO: Temporary code
	
	/*float x = myTransform.GetPosition().x;
	x += static_cast<float>(mySettings.swayAmplitude * std::sin(10 * myTimeLeft));
	myTransform.SetPosition({ x, myTransform.GetPosition().y, myTransform.GetPosition().y });*/
	// end


	// Setting position
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

void Particle::SetSize(const Tga::Vector2ui& aSize)
{
	mySize.x = static_cast<float>(aSize.x);
	mySize.y = static_cast<float>(aSize.y);
	myInstance.myTransform(1, 1) = static_cast<float>(mySize.x);
	myInstance.myTransform(2, 2) = static_cast<float>(mySize.y);
}

bool Particle::InUse() const
{
	return myTimeLeft > 0;
}
