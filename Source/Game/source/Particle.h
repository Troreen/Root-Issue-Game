#pragma once

#include <CommonUtilities/Vector.hpp>
#include <CommonUtilities/Transform.hpp>
#include <tge/sprite/sprite.h>
#include <tge/math/CommonMath.h>

enum class ParticleType
{
	Test,
	Blood,
	COUNT
};

struct ParticleSettings 
{
	float timeToLive = 0;
	float swayAmplitude = 0;
	float myOffset = 0;
	CommonUtilities::Vector2<float> size;
	CommonUtilities::Vector3<float> initalPosition;
	CommonUtilities::Vector3<float> linearVelocity;
	CommonUtilities::Vector4<float> startColor;
	CommonUtilities::Vector4<float> endColor;
	CommonUtilities::Quaternion<float> rotation;
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

	void SetSize(const Tga::Vector2ui& aSize);

	bool InUse() const;

private:

	float myTimeLeft;
	ParticleSettings mySettings;
	CommonUtilities::Transform<float> myTransform;
	
	Tga::Sprite3DInstanceData myInstance;

	CommonUtilities::Vector2<float> mySize;

	Particle* myNext;
};

