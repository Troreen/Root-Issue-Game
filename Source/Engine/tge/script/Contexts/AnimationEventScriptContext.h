#pragma once

#include <tge/script/Contexts/ScriptUpdateContext.h>
#include <tge/stringRegistry/StringRegistry.h>

namespace Tga
{
	struct AnimationEventCombatAttackDesc
	{
		StringId team;
		StringId attackType;
		StringId collisionShape;
		StringId targetLayer;
		float localOffsetX = 0.0f;
		float localOffsetY = 90.0f;
		float localOffsetZ = 0.0f;
		float sizeX = 0.0f;
		float sizeY = 0.0f;
		float sizeZ = 0.0f;
		float radius = 190.0f;
		float activeDurationSeconds = 0.16f;
		float knockbackStrength = 450.0f;
		int damage = 1;
		bool onlyHitForwardHemisphere = true;
	};

	struct AnimationEventScriptContext : public ScriptUpdateContext
	{
		StringId eventId;
		StringId clipPath;
		StringId scriptId;
		float eventTime = 0.0f;
		bool isDispatchingEvent = false;

		virtual void PlaySfx(StringId aSoundName) { aSoundName; }
		virtual void StartCombatAttack(const AnimationEventCombatAttackDesc& anAttack) { anAttack; }
		virtual void FirePlayerProjectile() {}
		virtual void TriggerCameraShake(float aDurationSeconds, float anIntensityUnits) { aDurationSeconds; anIntensityUnits; }
		virtual void LogAnimationEvent() const;
	};
}
