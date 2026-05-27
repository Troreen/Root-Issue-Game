#include "Particle.h"
#include "Essentials/Essentials.h"
#include <cassert>

void Particle::Init(const ParticleSettings& someParticleSettings)
{
	mySettings = someParticleSettings;
	myTransform.SetPosition(someParticleSettings.initalPosition);
	myTimeLeft = mySettings.timeToLive;

	myInstance.myTransform.SetForward(mySettings.rotation.GetForward().ToTga());
	myInstance.myTransform.SetRight(mySettings.rotation.GetRight().ToTga());
	myInstance.myTransform.SetUp(mySettings.rotation.GetUp().ToTga());

	//assert(mySize.LengthSqr() > 0);

	myInstance.myTransform(1, 1) *= static_cast<float>(mySize.x);
	myInstance.myTransform(1, 2) *= static_cast<float>(mySize.x);
	myInstance.myTransform(1, 3) *= static_cast<float>(mySize.x);
	myInstance.myTransform(2, 1) *= static_cast<float>(mySize.y);
	myInstance.myTransform(2, 2) *= static_cast<float>(mySize.y);
	myInstance.myTransform(2, 3) *= static_cast<float>(mySize.y);

	// Temp
	/*mySettings.swayAmplitudeX = 1;
	mySettings.swayAmplitudeZ = 3;
	mySettings.swayFrequencyX = 3;
	mySettings.swayFrequencyZ = 3;*/

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

	if (mySettings.sinAmplitudeX > 0 || mySettings.sinAmplitudeY > 0 || mySettings.sinAmplitudeZ > 0)
	{
		myTransform.SetPosition({
			myTransform.GetPosition().x + static_cast<float>(mySettings.sinAmplitudeX * std::sin(mySettings.sinFrequencyX * myTimeLeft + mySettings.myOffsetX)),
			myTransform.GetPosition().y + static_cast<float>(mySettings.sinAmplitudeY * std::sin(mySettings.sinFrequencyY * myTimeLeft + mySettings.myOffsetY)),
			myTransform.GetPosition().z + static_cast<float>(mySettings.sinAmplitudeZ * std::sin(mySettings.sinFrequencyZ * myTimeLeft + mySettings.myOffsetZ)) });
	}

	// Setting position
	mySettings.linearVelocity.y -= mySettings.gravity * aTimeDelta;
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
