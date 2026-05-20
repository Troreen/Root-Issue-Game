#include <stdafx.h>

#include "AnimationEventNodes.h"

#include <tge/script/BaseProperties.h>
#include <tge/script/Contexts/AnimationEventScriptContext.h>
#include <tge/script/Contexts/ScriptExecutionContext.h>
#include <tge/script/ScriptNodeBase.h>
#include <tge/script/ScriptNodeTypeRegistry.h>

namespace
{
	Tga::StringId ReadStringPin(Tga::ScriptExecutionContext& aContext, Tga::ScriptPinId aPin)
	{
		Tga::Property property = aContext.ReadInputPin(aPin);
		if (const Tga::StringId* value = property.Get<Tga::StringId>())
		{
			return *value;
		}

		return {};
	}

	float ReadFloatPin(Tga::ScriptExecutionContext& aContext, Tga::ScriptPinId aPin, float aDefaultValue)
	{
		Tga::Property property = aContext.ReadInputPin(aPin);
		if (const float* value = property.Get<float>())
		{
			return *value;
		}

		return aDefaultValue;
	}

	int ReadIntPin(Tga::ScriptExecutionContext& aContext, Tga::ScriptPinId aPin, int aDefaultValue)
	{
		Tga::Property property = aContext.ReadInputPin(aPin);
		if (const int* value = property.Get<int>())
		{
			return *value;
		}

		return aDefaultValue;
	}

	bool ReadBoolPin(Tga::ScriptExecutionContext& aContext, Tga::ScriptPinId aPin, bool aDefaultValue)
	{
		Tga::Property property = aContext.ReadInputPin(aPin);
		if (const bool* value = property.Get<bool>())
		{
			return *value;
		}

		return aDefaultValue;
	}

	Tga::AnimationEventScriptContext* GetEventContext(Tga::ScriptExecutionContext& aContext)
	{
		return dynamic_cast<Tga::AnimationEventScriptContext*>(&aContext.GetUpdateContext());
	}

	class OnAnimationEventNode final : public Tga::ScriptNodeBase
	{
	public:
		void Init(const Tga::ScriptCreationContext& aContext) override
		{
			Tga::ScriptPin inputPin = {};
			inputPin.type = Tga::ScriptLinkType::Flow;
			inputPin.name = "Event"_tgaid;
			inputPin.node = aContext.GetNodeId();
			inputPin.role = Tga::ScriptPinRole::Input;
			myInputPin = aContext.FindOrCreatePin(inputPin);

			Tga::ScriptPin outputPin = {};
			outputPin.type = Tga::ScriptLinkType::Flow;
			outputPin.name = "Out"_tgaid;
			outputPin.node = aContext.GetNodeId();
			outputPin.role = Tga::ScriptPinRole::Output;
			myOutputPin = aContext.FindOrCreatePin(outputPin);

			Tga::ScriptPin eventIdPin = {};
			eventIdPin.type = Tga::ScriptLinkType::Property;
			eventIdPin.dataType = Tga::GetPropertyType<Tga::StringId>();
			eventIdPin.name = "Event Id"_tgaid;
			eventIdPin.node = aContext.GetNodeId();
			eventIdPin.role = Tga::ScriptPinRole::Input;
			eventIdPin.defaultValue = Tga::Property::Create<Tga::StringId>();
			myEventIdPin = aContext.FindOrCreatePin(eventIdPin);
		}

		Tga::ScriptNodeResult Execute(Tga::ScriptExecutionContext& aContext, Tga::ScriptPinId) const override
		{
			Tga::AnimationEventScriptContext* context = GetEventContext(aContext);
			if (!context || !context->isDispatchingEvent)
			{
				return Tga::ScriptNodeResult::Finished;
			}

			const Tga::StringId requiredEventId = ReadStringPin(aContext, myEventIdPin);
			if (!requiredEventId.IsEmpty() && requiredEventId != context->eventId)
			{
				return Tga::ScriptNodeResult::Finished;
			}

			aContext.TriggerOutputPin(myOutputPin);
			return Tga::ScriptNodeResult::Finished;
		}

	private:
		Tga::ScriptPinId myInputPin;
		Tga::ScriptPinId myOutputPin;
		Tga::ScriptPinId myEventIdPin;
	};

	class PlaySfxNode final : public Tga::ScriptNodeBase
	{
	public:
		void Init(const Tga::ScriptCreationContext& aContext) override
		{
			Tga::ScriptPin flowIn = {};
			flowIn.type = Tga::ScriptLinkType::Flow;
			flowIn.name = "In"_tgaid;
			flowIn.node = aContext.GetNodeId();
			flowIn.role = Tga::ScriptPinRole::Input;
			myFlowInPin = aContext.FindOrCreatePin(flowIn);

			Tga::ScriptPin flowOut = {};
			flowOut.type = Tga::ScriptLinkType::Flow;
			flowOut.name = "Out"_tgaid;
			flowOut.node = aContext.GetNodeId();
			flowOut.role = Tga::ScriptPinRole::Output;
			myFlowOutPin = aContext.FindOrCreatePin(flowOut);

			Tga::ScriptPin soundPin = {};
			soundPin.type = Tga::ScriptLinkType::Property;
			soundPin.dataType = Tga::GetPropertyType<Tga::StringId>();
			soundPin.name = "Sound"_tgaid;
			soundPin.node = aContext.GetNodeId();
			soundPin.role = Tga::ScriptPinRole::Input;
			soundPin.defaultValue = Tga::Property::Create<Tga::StringId>("VineBoom"_tgaid);
			mySoundPin = aContext.FindOrCreatePin(soundPin);
		}

		Tga::ScriptNodeResult Execute(Tga::ScriptExecutionContext& aContext, Tga::ScriptPinId) const override
		{
			if (Tga::AnimationEventScriptContext* context = GetEventContext(aContext))
			{
				context->PlaySfx(ReadStringPin(aContext, mySoundPin));
			}

			aContext.TriggerOutputPin(myFlowOutPin);
			return Tga::ScriptNodeResult::Finished;
		}

	private:
		Tga::ScriptPinId myFlowInPin;
		Tga::ScriptPinId myFlowOutPin;
		Tga::ScriptPinId mySoundPin;
	};

	class StartCombatAttackNode final : public Tga::ScriptNodeBase
	{
	public:
		void Init(const Tga::ScriptCreationContext& aContext) override
		{
			Tga::ScriptPin flowIn = {};
			flowIn.type = Tga::ScriptLinkType::Flow;
			flowIn.name = "In"_tgaid;
			flowIn.node = aContext.GetNodeId();
			flowIn.role = Tga::ScriptPinRole::Input;
			myFlowInPin = aContext.FindOrCreatePin(flowIn);

			Tga::ScriptPin flowOut = {};
			flowOut.type = Tga::ScriptLinkType::Flow;
			flowOut.name = "Out"_tgaid;
			flowOut.node = aContext.GetNodeId();
			flowOut.role = Tga::ScriptPinRole::Output;
			myFlowOutPin = aContext.FindOrCreatePin(flowOut);

			Tga::ScriptPin teamPin = {};
			teamPin.type = Tga::ScriptLinkType::Property;
			teamPin.dataType = Tga::GetPropertyType<Tga::StringId>();
			teamPin.name = "Team"_tgaid;
			teamPin.node = aContext.GetNodeId();
			teamPin.role = Tga::ScriptPinRole::Input;
			teamPin.defaultValue = Tga::Property::Create<Tga::StringId>("Owner"_tgaid);
			myTeamPin = aContext.FindOrCreatePin(teamPin);

			Tga::ScriptPin attackTypePin = {};
			attackTypePin.type = Tga::ScriptLinkType::Property;
			attackTypePin.dataType = Tga::GetPropertyType<Tga::StringId>();
			attackTypePin.name = "Attack Type"_tgaid;
			attackTypePin.node = aContext.GetNodeId();
			attackTypePin.role = Tga::ScriptPinRole::Input;
			attackTypePin.defaultValue = Tga::Property::Create<Tga::StringId>("MeleeLight"_tgaid);
			myAttackTypePin = aContext.FindOrCreatePin(attackTypePin);

			Tga::ScriptPin shapePin = {};
			shapePin.type = Tga::ScriptLinkType::Property;
			shapePin.dataType = Tga::GetPropertyType<Tga::StringId>();
			shapePin.name = "Shape"_tgaid;
			shapePin.node = aContext.GetNodeId();
			shapePin.role = Tga::ScriptPinRole::Input;
			shapePin.defaultValue = Tga::Property::Create<Tga::StringId>("Sphere"_tgaid);
			myShapePin = aContext.FindOrCreatePin(shapePin);

			Tga::ScriptPin targetLayerPin = {};
			targetLayerPin.type = Tga::ScriptLinkType::Property;
			targetLayerPin.dataType = Tga::GetPropertyType<Tga::StringId>();
			targetLayerPin.name = "Target Layer"_tgaid;
			targetLayerPin.node = aContext.GetNodeId();
			targetLayerPin.role = Tga::ScriptPinRole::Input;
			targetLayerPin.defaultValue = Tga::Property::Create<Tga::StringId>("Opposing"_tgaid);
			myTargetLayerPin = aContext.FindOrCreatePin(targetLayerPin);

			Tga::ScriptPin damagePin = {};
			damagePin.type = Tga::ScriptLinkType::Property;
			damagePin.dataType = Tga::GetPropertyType<int>();
			damagePin.name = "Damage"_tgaid;
			damagePin.node = aContext.GetNodeId();
			damagePin.role = Tga::ScriptPinRole::Input;
			damagePin.defaultValue = Tga::Property::Create<int>(1);
			myDamagePin = aContext.FindOrCreatePin(damagePin);

			Tga::ScriptPin radiusPin = {};
			radiusPin.type = Tga::ScriptLinkType::Property;
			radiusPin.dataType = Tga::GetPropertyType<float>();
			radiusPin.name = "Radius"_tgaid;
			radiusPin.node = aContext.GetNodeId();
			radiusPin.role = Tga::ScriptPinRole::Input;
			radiusPin.defaultValue = Tga::Property::Create<float>(190.0f);
			myRadiusPin = aContext.FindOrCreatePin(radiusPin);

			Tga::ScriptPin durationPin = {};
			durationPin.type = Tga::ScriptLinkType::Property;
			durationPin.dataType = Tga::GetPropertyType<float>();
			durationPin.name = "Duration"_tgaid;
			durationPin.node = aContext.GetNodeId();
			durationPin.role = Tga::ScriptPinRole::Input;
			durationPin.defaultValue = Tga::Property::Create<float>(0.16f);
			myDurationPin = aContext.FindOrCreatePin(durationPin);

			Tga::ScriptPin knockbackPin = {};
			knockbackPin.type = Tga::ScriptLinkType::Property;
			knockbackPin.dataType = Tga::GetPropertyType<float>();
			knockbackPin.name = "Knockback"_tgaid;
			knockbackPin.node = aContext.GetNodeId();
			knockbackPin.role = Tga::ScriptPinRole::Input;
			knockbackPin.defaultValue = Tga::Property::Create<float>(450.0f);
			myKnockbackPin = aContext.FindOrCreatePin(knockbackPin);

			Tga::ScriptPin onlyForwardPin = {};
			onlyForwardPin.type = Tga::ScriptLinkType::Property;
			onlyForwardPin.dataType = Tga::GetPropertyType<bool>();
			onlyForwardPin.name = "Only Forward"_tgaid;
			onlyForwardPin.node = aContext.GetNodeId();
			onlyForwardPin.role = Tga::ScriptPinRole::Input;
			onlyForwardPin.defaultValue = Tga::Property::Create<bool>(true);
			myOnlyForwardPin = aContext.FindOrCreatePin(onlyForwardPin);

			myOffsetXPin = CreateFloatPin(aContext, "Offset X"_tgaid, 0.0f);
			myOffsetYPin = CreateFloatPin(aContext, "Offset Y"_tgaid, 90.0f);
			myOffsetZPin = CreateFloatPin(aContext, "Offset Z"_tgaid, 0.0f);
			mySizeXPin = CreateFloatPin(aContext, "Size X"_tgaid, 0.0f);
			mySizeYPin = CreateFloatPin(aContext, "Size Y"_tgaid, 0.0f);
			mySizeZPin = CreateFloatPin(aContext, "Size Z"_tgaid, 0.0f);
		}

		Tga::ScriptNodeResult Execute(Tga::ScriptExecutionContext& aContext, Tga::ScriptPinId) const override
		{
			if (Tga::AnimationEventScriptContext* context = GetEventContext(aContext))
			{
				Tga::AnimationEventCombatAttackDesc attack;
				attack.team = ReadStringPin(aContext, myTeamPin);
				attack.attackType = ReadStringPin(aContext, myAttackTypePin);
				attack.collisionShape = ReadStringPin(aContext, myShapePin);
				attack.targetLayer = ReadStringPin(aContext, myTargetLayerPin);
				attack.localOffsetX = ReadFloatPin(aContext, myOffsetXPin, 0.0f);
				attack.localOffsetY = ReadFloatPin(aContext, myOffsetYPin, 90.0f);
				attack.localOffsetZ = ReadFloatPin(aContext, myOffsetZPin, 0.0f);
				attack.sizeX = ReadFloatPin(aContext, mySizeXPin, 0.0f);
				attack.sizeY = ReadFloatPin(aContext, mySizeYPin, 0.0f);
				attack.sizeZ = ReadFloatPin(aContext, mySizeZPin, 0.0f);
				attack.radius = ReadFloatPin(aContext, myRadiusPin, 190.0f);
				attack.activeDurationSeconds = ReadFloatPin(aContext, myDurationPin, 0.16f);
				attack.knockbackStrength = ReadFloatPin(aContext, myKnockbackPin, 450.0f);
				attack.damage = ReadIntPin(aContext, myDamagePin, 1);
				attack.onlyHitForwardHemisphere = ReadBoolPin(aContext, myOnlyForwardPin, true);
				context->StartCombatAttack(attack);
			}

			aContext.TriggerOutputPin(myFlowOutPin);
			return Tga::ScriptNodeResult::Finished;
		}

	private:
		Tga::ScriptPinId CreateFloatPin(const Tga::ScriptCreationContext& aContext, Tga::StringId aName, float aDefaultValue)
		{
			Tga::ScriptPin pin = {};
			pin.type = Tga::ScriptLinkType::Property;
			pin.dataType = Tga::GetPropertyType<float>();
			pin.name = aName;
			pin.node = aContext.GetNodeId();
			pin.role = Tga::ScriptPinRole::Input;
			pin.defaultValue = Tga::Property::Create<float>(aDefaultValue);
			return aContext.FindOrCreatePin(pin);
		}

		Tga::ScriptPinId myFlowInPin;
		Tga::ScriptPinId myFlowOutPin;
		Tga::ScriptPinId myTeamPin;
		Tga::ScriptPinId myAttackTypePin;
		Tga::ScriptPinId myShapePin;
		Tga::ScriptPinId myTargetLayerPin;
		Tga::ScriptPinId myDamagePin;
		Tga::ScriptPinId myRadiusPin;
		Tga::ScriptPinId myDurationPin;
		Tga::ScriptPinId myKnockbackPin;
		Tga::ScriptPinId myOnlyForwardPin;
		Tga::ScriptPinId myOffsetXPin;
		Tga::ScriptPinId myOffsetYPin;
		Tga::ScriptPinId myOffsetZPin;
		Tga::ScriptPinId mySizeXPin;
		Tga::ScriptPinId mySizeYPin;
		Tga::ScriptPinId mySizeZPin;
	};

	class FirePlayerProjectileNode final : public Tga::ScriptNodeBase
	{
	public:
		void Init(const Tga::ScriptCreationContext& aContext) override
		{
			Tga::ScriptPin flowIn = {};
			flowIn.type = Tga::ScriptLinkType::Flow;
			flowIn.name = "In"_tgaid;
			flowIn.node = aContext.GetNodeId();
			flowIn.role = Tga::ScriptPinRole::Input;
			myFlowInPin = aContext.FindOrCreatePin(flowIn);

			Tga::ScriptPin flowOut = {};
			flowOut.type = Tga::ScriptLinkType::Flow;
			flowOut.name = "Out"_tgaid;
			flowOut.node = aContext.GetNodeId();
			flowOut.role = Tga::ScriptPinRole::Output;
			myFlowOutPin = aContext.FindOrCreatePin(flowOut);
		}

		Tga::ScriptNodeResult Execute(Tga::ScriptExecutionContext& aContext, Tga::ScriptPinId) const override
		{
			if (Tga::AnimationEventScriptContext* context = GetEventContext(aContext))
			{
				context->FirePlayerProjectile();
			}

			aContext.TriggerOutputPin(myFlowOutPin);
			return Tga::ScriptNodeResult::Finished;
		}

	private:
		Tga::ScriptPinId myFlowInPin;
		Tga::ScriptPinId myFlowOutPin;
	};

	class CameraShakeNode final : public Tga::ScriptNodeBase
	{
	public:
		void Init(const Tga::ScriptCreationContext& aContext) override
		{
			Tga::ScriptPin flowIn = {};
			flowIn.type = Tga::ScriptLinkType::Flow;
			flowIn.name = "In"_tgaid;
			flowIn.node = aContext.GetNodeId();
			flowIn.role = Tga::ScriptPinRole::Input;
			myFlowInPin = aContext.FindOrCreatePin(flowIn);

			Tga::ScriptPin flowOut = {};
			flowOut.type = Tga::ScriptLinkType::Flow;
			flowOut.name = "Out"_tgaid;
			flowOut.node = aContext.GetNodeId();
			flowOut.role = Tga::ScriptPinRole::Output;
			myFlowOutPin = aContext.FindOrCreatePin(flowOut);

			Tga::ScriptPin durationPin = {};
			durationPin.type = Tga::ScriptLinkType::Property;
			durationPin.dataType = Tga::GetPropertyType<float>();
			durationPin.name = "Duration"_tgaid;
			durationPin.node = aContext.GetNodeId();
			durationPin.role = Tga::ScriptPinRole::Input;
			durationPin.defaultValue = Tga::Property::Create<float>(0.15f);
			myDurationPin = aContext.FindOrCreatePin(durationPin);

			Tga::ScriptPin intensityPin = {};
			intensityPin.type = Tga::ScriptLinkType::Property;
			intensityPin.dataType = Tga::GetPropertyType<float>();
			intensityPin.name = "Intensity"_tgaid;
			intensityPin.node = aContext.GetNodeId();
			intensityPin.role = Tga::ScriptPinRole::Input;
			intensityPin.defaultValue = Tga::Property::Create<float>(20.0f);
			myIntensityPin = aContext.FindOrCreatePin(intensityPin);
		}

		Tga::ScriptNodeResult Execute(Tga::ScriptExecutionContext& aContext, Tga::ScriptPinId) const override
		{
			if (Tga::AnimationEventScriptContext* context = GetEventContext(aContext))
			{
				context->TriggerCameraShake(
					ReadFloatPin(aContext, myDurationPin, 0.15f),
					ReadFloatPin(aContext, myIntensityPin, 20.0f));
			}

			aContext.TriggerOutputPin(myFlowOutPin);
			return Tga::ScriptNodeResult::Finished;
		}

	private:
		Tga::ScriptPinId myFlowInPin;
		Tga::ScriptPinId myFlowOutPin;
		Tga::ScriptPinId myDurationPin;
		Tga::ScriptPinId myIntensityPin;
	};

	class LogAnimationEventNode final : public Tga::ScriptNodeBase
	{
	public:
		void Init(const Tga::ScriptCreationContext& aContext) override
		{
			Tga::ScriptPin flowIn = {};
			flowIn.type = Tga::ScriptLinkType::Flow;
			flowIn.name = "In"_tgaid;
			flowIn.node = aContext.GetNodeId();
			flowIn.role = Tga::ScriptPinRole::Input;
			myFlowInPin = aContext.FindOrCreatePin(flowIn);

			Tga::ScriptPin flowOut = {};
			flowOut.type = Tga::ScriptLinkType::Flow;
			flowOut.name = "Out"_tgaid;
			flowOut.node = aContext.GetNodeId();
			flowOut.role = Tga::ScriptPinRole::Output;
			myFlowOutPin = aContext.FindOrCreatePin(flowOut);
		}

		Tga::ScriptNodeResult Execute(Tga::ScriptExecutionContext& aContext, Tga::ScriptPinId) const override
		{
			if (Tga::AnimationEventScriptContext* context = GetEventContext(aContext))
			{
				context->LogAnimationEvent();
			}

			aContext.TriggerOutputPin(myFlowOutPin);
			return Tga::ScriptNodeResult::Finished;
		}

	private:
		Tga::ScriptPinId myFlowInPin;
		Tga::ScriptPinId myFlowOutPin;
	};
}

void Tga::RegisterAnimationEventNodes()
{
	static bool isRegistered = false;
	if (isRegistered)
	{
		return;
	}

	ScriptNodeTypeRegistry::RegisterType<OnAnimationEventNode>(
		"Animation Event/On Animation Event",
		"Entry point for scripts bound to animation event markers.");
	ScriptNodeTypeRegistry::RegisterType<PlaySfxNode>(
		"Animation Event/Audio/Play SFX",
		"Plays a named sound effect.");
	ScriptNodeTypeRegistry::RegisterType<StartCombatAttackNode>(
		"Animation Event/Gameplay/Start Combat Attack",
		"Starts a gameplay-owned combat attack using the animated object as owner.");
	ScriptNodeTypeRegistry::RegisterType<FirePlayerProjectileNode>(
		"Animation Event/Gameplay/Fire Player Projectile",
		"Fires the player's configured projectile from the animated player object.");
	ScriptNodeTypeRegistry::RegisterType<CameraShakeNode>(
		"Animation Event/Camera/Camera Shake",
		"Triggers the global gameplay camera shake.");
	ScriptNodeTypeRegistry::RegisterType<LogAnimationEventNode>(
		"Animation Event/Debug/Log Animation Event",
		"Logs the current animation event.");

	isRegistered = true;
}

std::span<const char* const> Tga::GetAnimationEventSfxIds()
{
	static constexpr const char* kSfxIds[] =
	{
		"VineBoom",
		"BasicVox",
		"BasicAttackVox",
		"HeavyVox",
		"RollBegin",
		"Roll",
		"PlayerAttack",
		"EnemyDeadVox",
		"Charge",
		"Shoot",
		"Gore",
	};

	return kSfxIds;
}

std::span<const char* const> Tga::GetAnimationEventCombatTeamIds()
{
	static constexpr const char* kTeamIds[] =
	{
		"Owner",
		"Player",
		"Enemy",
		"Neutral",
	};

	return kTeamIds;
}

std::span<const char* const> Tga::GetAnimationEventAttackTypeIds()
{
	static constexpr const char* kAttackTypeIds[] =
	{
		"MeleeLight",
		"MeleeCombo",
		"MeleeCharged",
		"DodgeAttack",
		"Ranged",
		"EnemyMelee",
		"EnemyRoll",
	};

	return kAttackTypeIds;
}

std::span<const char* const> Tga::GetAnimationEventCollisionShapeIds()
{
	static constexpr const char* kShapeIds[] =
	{
		"Sphere",
		"Box",
	};

	return kShapeIds;
}

std::span<const char* const> Tga::GetAnimationEventTargetLayerIds()
{
	static constexpr const char* kTargetLayerIds[] =
	{
		"Opposing",
		"Player",
		"Enemy",
		"WorldStatic",
		"Projectile",
		"Trigger",
		"Pickup",
		"NPC",
		"Switch",
	};

	return kTargetLayerIds;
}
