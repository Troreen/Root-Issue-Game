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
    Line,
    COUNT
};

struct ParticleEmitterSettings
{
    //TODO: Set these to default values that make sense!

    EmissionShape shape;

    CommonUtilities::Vector3<float> emissionDir = { 0.f,0.f,1.f };
    bool directionModified = false;

    float emissionRate = 3000.f; // per second
    float emissionAccumulator = 0;

    float lifeTimeMin = 0.f;
    float lifeTimeMax = 1.f;

    float startSpeedMin = 0;
    float startSpeedMax = 500.f;

    float coneAngle = 45.f;

    float startSizeMin = 50.f;
    float startSizeMax = 1.f;
    float endSizeMin = 2.f;
    float endSizeMax = 3.f;

    float burstCount = 3000;
    bool shouldBurst = false;

    bool shouldEmitContinuously = false;
    bool startActive = false;
};

class ParticleEmitterComponent : public Component
{
public:

    ParticleEmitterComponent();

    void Init(Tga::Engine& anEngine) override;
    void Update(float aDeltaTime) override;

    void AddParticleWithShape(const ParticleType& aParticleType, const EmissionShape& aShape);
    
    void Burst(const ParticleType& aParticleType);

    void SetEmissionDirection(const ParticleType& aParticleType, const CommonUtilities::Vector3<float>& aDirection);
    void SetContinuousEmission(const ParticleType& aParticleType, bool aStatus);

private:
	
    void Emit(const ParticleType& aParticleType, const CommonUtilities::Transform<float>& aTransform);

    std::unordered_map<ParticleType, ParticleEmitterSettings> mySettingsCollection;

    //ParticleEmitterSettings mySettings;

    //float myEmissionAccumulator;

    //bool myShouldBurst;
    //bool myDirectionModified;
};

