#pragma once
#include "Component.h"

#include <CommonUtilities/Vector3.hpp>
#include <CommonUtilities/Vector4.hpp>
#include <unordered_map>

#include "Particle.h"


enum class EmissionShape
{
    Box,
    Sphere,
    Cone,
    COUNT
};

struct ParticleEmitterSettings
{
    //TODO: Set these to default values that make sense!

    EmissionShape shape;

    bool shouldBillboard = true;

    CommonUtilities::Vector3<float> emissionDir = { 0.f,1.f,0.f };
    bool directionModified = false;

    float emissionRate = 10.f; // per second
    float emissionAccumulator = 0;

    float lifeTimeMin = 0.f;
    float lifeTimeMax = 1.f;

    float startSpeedMin = 0;
    float startSpeedMax = 500.f;
    float gravity = 0;

    float coneAngle = 45.f;

    CommonUtilities::Vector3<float> spawnOffset = { 0.f,0.f,0.f };
    bool spawnOffsetIsLocal = true;

    CommonUtilities::Vector3<float> boxBounds = { 0,0,0 };

    float startSizeMin = 50.f;
    float startSizeMax = 1.f;
    float endSizeMin = 2.f;
    float endSizeMax = 3.f;

    float sinAmplitudeX = 0;
    float sinAmplitudeY = 0;
    float sinAmplitudeZ = 0;
    float sinFrequencyX = 0;
    float sinFrequenxyY = 0;
    float sinFrequencyZ = 0;
    bool sinSynchronized = true;

    float burstCount = 3000;
    bool shouldBurst = false;
    bool shouldStartBurstign = false;

    bool shouldEmitContinuously = false;
    bool startActive = false;
    float emissionDuration = 0;
};

class ParticleEmitterComponent : public Component
{
public:

    ParticleEmitterComponent();

    void AttachSettings();

    void Init(Tga::Engine& anEngine) override;
    void Update(float aDeltaTime) override;

    void Burst(const ParticleType& aParticleType);

    void SetOffset(const ParticleType& aParticleType, const CommonUtilities::Vector3<float>& anOffset);

    void SetEmissionDirection(const ParticleType& aParticleType, const CommonUtilities::Vector3<float>& aDirection);
    void SetContinuousEmission(const ParticleType& aParticleType, bool aStatus);
    void SetEmissionWithDuration(const ParticleType& aParticleType, float aDuration);

private:
	
    void Emit(const ParticleType& aParticleType, const CommonUtilities::Transform<float>& aTransform);

    std::unordered_map<ParticleType, ParticleEmitterSettings> mySettingsCollection;

    //ParticleEmitterSettings mySettings;

    //float myEmissionAccumulator;

    //bool myShouldBurst;
    //bool myDirectionModified;
};

