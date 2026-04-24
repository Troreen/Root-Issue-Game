#pragma once

#include <CommonUtilities/Vector.hpp>
#include <tge/sprite/sprite.h>
#include <tge/math/Matrix4x4.h>


struct ParticleSettings 
{
	float timeToLive;
	CommonUtilities::Vector3<float> initalPosition;
	CommonUtilities::Vector3<float> linearVelocity;
	CommonUtilities::Vector2<float> size;
	CommonUtilities::Vector4<float> startColor;
	CommonUtilities::Vector4<float> endColor;
};

class Particle
{
public:
	Particle() = default;
	~Particle() = default;

	void Init(const ParticleSettings& someParticleSettings);

	void SetNext(Particle* aNext);
	Particle* GetNext() const;

	bool Animate(float aTimeDelta);

	const Tga::Sprite3DInstanceData& GetInstance() const;

	bool InUse() const;

private:

	float myTimeLeft;
	ParticleSettings mySettings;
	Tga::Matrix4x4f myTransform;
	Tga::Sprite3DInstanceData myInstance;

	Particle* myNext;
};

