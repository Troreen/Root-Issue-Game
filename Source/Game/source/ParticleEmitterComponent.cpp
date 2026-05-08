#include "ParticleEmitterComponent.h"

#include "GameObject.h"
#include "VfxSystem.h"

#include "Essentials/Essentials.h"

#include <random>

constexpr auto DEGREETORADIAN = 0.017453292f;
constexpr float PI = 3.14159f;

ParticleEmitterComponent::ParticleEmitterComponent()
{
}

void ParticleEmitterComponent::AttachSettings()
{
	std::string ownerName = GetOwner()->GetName();
	//std::cout << "Looking for emitter settings for: " << ownerName << std::endl;
	mySettingsCollection = *VfxService::Get()->GetEmissionSettingsForObject(ownerName);
	for (auto it = mySettingsCollection.begin(); it != mySettingsCollection.end(); ++it)
	{
		it->second.shouldEmitContinuously = it->second.startActive;
		it->second.shouldBurst = false;
	}
}

void ParticleEmitterComponent::Init(Tga::Engine& anEngine)
{
	anEngine;
}

void ParticleEmitterComponent::Update(float aDeltaTime)
{
	for (auto it = mySettingsCollection.begin(); it != mySettingsCollection.end(); ++it)
	{
		if (it->second.shouldBurst || it->second.shouldEmitContinuously)
		{
			// TODO: Temporary code
			CommonUtilities::Transform<float> transform = Essentials::globalCamera.get()->GetCamera().GetTransform();
			transform.SetPosition(GetOwner()->GetTransform().GetPosition());
			//CommonUtilities::Transform<float> transform = GetOwner()->GetTransform();
			// End

			if (it->second.shouldBurst)
			{
				for (int i = 0; i < it->second.burstCount; ++i)
				{
					Emit(it->first, transform);
				}

				it->second.shouldBurst = false;
			}

			if (it->second.shouldEmitContinuously)
			{
				it->second.emissionAccumulator += aDeltaTime * it->second.emissionRate;

				int nbrToEmit = static_cast<int>(it->second.emissionAccumulator);
				it->second.emissionAccumulator -= nbrToEmit;

				for (int i = 0; i < nbrToEmit; ++i)
				{
					Emit(it->first, transform);
				}
			}
		}
	}
}

void ParticleEmitterComponent::Burst(const ParticleType& aParticleType)
{
	assert(mySettingsCollection.contains(aParticleType));
	mySettingsCollection[aParticleType].shouldBurst = true;
}

void ParticleEmitterComponent::SetEmissionDirection(const ParticleType& aParticleType, const CommonUtilities::Vector3<float>& aDirection)
{
	assert(mySettingsCollection.contains(aParticleType));
	mySettingsCollection[aParticleType].emissionDir = aDirection;
	//mySettingsCollection[aParticleType].directionModified = true; // Is this even needed????
}

void ParticleEmitterComponent::SetContinuousEmission(const ParticleType& aParticleType, bool aStatus)
{
	mySettingsCollection[aParticleType].shouldEmitContinuously = aStatus;
}

void inline ParticleEmitterComponent::Emit(const ParticleType& aParticleType, const CommonUtilities::Transform<float>& aTransform)
{
	static std::mt19937 rng(std::random_device{}());
	static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
	static std::uniform_real_distribution<float> dist01(0.f, 1.f);

	Vector3f forward = mySettingsCollection[aParticleType].emissionDir.GetNormalized();
	Vector3f worldUp = fabs(forward.y) < 0.999f ? Vector3f(0, 1, 0) : Vector3f(1, 0, 0);
	Vector3f right = worldUp.Cross(forward).GetNormalized();
	Vector3f up = forward.Cross(right);

	ParticleSettings settings;
	ParticleEmitterSettings* emitterSettings = &mySettingsCollection[aParticleType];

	switch (emitterSettings->shape)
	{
	case EmissionShape::Box:
	{
		float spread = 50.0f;

		Vector3f randomOffset = {
			dist(rng),
			dist(rng),
			dist(rng)
		};

		randomOffset *= spread;

		settings = {
			.timeToLive = emitterSettings->lifeTimeMax,
			.initalPosition = aTransform.GetPosition() + randomOffset,
			.linearVelocity = forward * emitterSettings->startSpeedMax,
			.rotation = aTransform.GetRotation()
		};
		break;
	}

	case EmissionShape::Sphere:
	{
		float theta = dist01(rng) * 2 * PI;

		float z = dist(rng);

		float r = std::sqrt(1 - z * z);

		float x = r * std::cos(theta);
		float y = r * std::sin(theta);

		Vector3f dir = { x,y,z };

		settings = {
			.timeToLive = emitterSettings->lifeTimeMax,
			.initalPosition = aTransform.GetPosition(),
			.linearVelocity = dir * emitterSettings->startSpeedMax,
			.rotation = aTransform.GetRotation()
		};

		break;
	}
	case EmissionShape::Cone:
	{

		float angleRad = emitterSettings->coneAngle * DEGREETORADIAN;

		float u = dist01(rng);   // [0,1]
		float v = dist01(rng);   // [0,1]

		float cosTheta = (1.0f - u) + u * cos(angleRad);
		float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

		float omega = 2.0f * PI * v;

		Vector3f finalDir =
			right * (cos(omega) * sinTheta) +
			up * (sin(omega) * sinTheta) +
			forward * cosTheta;

		finalDir.Normalize();

		float speed = emitterSettings->startSpeedMax;

		settings = {
			.timeToLive = emitterSettings->lifeTimeMax,
			.initalPosition = aTransform.GetPosition(),
			.linearVelocity = finalDir * speed
		};
		break;
	}
	default:
		return;
		break;
	}

	if (emitterSettings->lifeTimeMax != emitterSettings->lifeTimeMin)
	{
		std::uniform_real_distribution<float> lifeDist(emitterSettings->lifeTimeMin, emitterSettings->lifeTimeMax);
		settings.timeToLive = lifeDist(rng);
	}
	else
	{
		settings.timeToLive = emitterSettings->lifeTimeMin;
	}

	/*if (emitterSettings->startSpeedMin != emitterSettings->startSpeedMax)
	{
		std::uniform_real_distribution<float> speedDist(emitterSettings->startSpeedMin, emitterSettings->startSpeedMax);
		settings.linearVelocity = speedDist(rng);
	}
	else
	{
		settings.linearVelocity = emitterSettings->startSpeedMin;
	}*/

	//settings.size = mySettingsCollection[aParticleType].startSizeMax;
	VfxService::SpawnParticle(aParticleType, settings);
}

