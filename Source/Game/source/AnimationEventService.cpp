#include "AnimationEventService.h"

#include "AudioManager.h"
#include "CameraSystem.h"
#include "CombatSystem.h"
#include "Essentials.h"
#include "GameObject.h"
#include "PlayerControllerComponent.h"

#include <tge/engine.h>
#include <tge/script/Script.h>
#include <tge/script/ScriptManager.h>
#include <tge/script/ScriptNodeTypeRegistry.h>

#include <algorithm>
#include <cstring>
#include <iostream>

namespace
{
	bool MatchesText(const Tga::StringId aValue, const char* aText)
	{
		return !aValue.IsEmpty() && std::strcmp(aValue.GetString(), aText) == 0;
	}

	bool TryResolveSoundId(const Tga::StringId aSoundName, SoundID& outSoundId)
	{
		if (MatchesText(aSoundName, "VineBoom") || MatchesText(aSoundName, "eVineBoom"))
		{
			outSoundId = SoundID::eVineBoom;
			return true;
		}

		if (MatchesText(aSoundName, "MusicLoop") || MatchesText(aSoundName, "eMusicLoop"))
		{
			outSoundId = SoundID::eMusicLoop;
			return true;
		}

		if (MatchesText(aSoundName, "Step") || MatchesText(aSoundName, "eStep"))
		{
			outSoundId = SoundID::eStep;
			return true;
		}

		if (MatchesText(aSoundName, "BasicVox") || MatchesText(aSoundName, "eBasicVox"))
		{
			outSoundId = SoundID::eBasicVox;
			return true;
		}

		if (MatchesText(aSoundName, "HeavyVox") || MatchesText(aSoundName, "eHeavyVox"))
		{
			outSoundId = SoundID::eHeavyVox;
			return true;
		}

		if (MatchesText(aSoundName, "Charge") || MatchesText(aSoundName, "eCharge"))
		{
			outSoundId = SoundID::eCharge;
			return true;
		}

		if (MatchesText(aSoundName, "Shoot") || MatchesText(aSoundName, "eShoot"))
		{
			outSoundId = SoundID::eShoot;
			return true;
		}

		if (MatchesText(aSoundName, "RollBegin") || MatchesText(aSoundName, "eRollBegin"))
		{
			outSoundId = SoundID::eRollBegin;
			return true;
		}

		if (MatchesText(aSoundName, "Roll") || MatchesText(aSoundName, "eRoll"))
		{
			outSoundId = SoundID::eRoll;
			return true;
		}

		if (MatchesText(aSoundName, "PlayerAttack") || MatchesText(aSoundName, "ePlayerAttack"))
		{
			outSoundId = SoundID::ePlayerAttack;
			return true;
		}

		if (MatchesText(aSoundName, "EnemyDeadVox") || MatchesText(aSoundName, "eEnemyDeadVox"))
		{
			outSoundId = SoundID::eEnemyDeadVox;
			return true;
		}

		if (MatchesText(aSoundName, "BasicAttackVox") || MatchesText(aSoundName, "eBasicAttackVox"))
		{
			outSoundId = SoundID::eBasicAttackVox;
			return true;
		}

		if (MatchesText(aSoundName, "Gore") || MatchesText(aSoundName, "eGore"))
		{
			outSoundId = SoundID::eGore;
			return true;
		}

		return false;
	}

	CombatTeam ResolveOwnerTeam(const GameObject& anOwner)
	{
		switch (anOwner.GetLayer())
		{
		case ObjectLayer::Player:
			return CombatTeam::Player;
		case ObjectLayer::Enemy:
			return CombatTeam::Enemy;
		default:
			return CombatTeam::Neutral;
		}
	}

	CombatTeam ResolveCombatTeam(const Tga::StringId aTeamName, const GameObject& anOwner)
	{
		if (aTeamName.IsEmpty() || MatchesText(aTeamName, "Owner") || MatchesText(aTeamName, "Auto"))
		{
			return ResolveOwnerTeam(anOwner);
		}

		if (MatchesText(aTeamName, "Player"))
		{
			return CombatTeam::Player;
		}

		if (MatchesText(aTeamName, "Enemy"))
		{
			return CombatTeam::Enemy;
		}

		return CombatTeam::Neutral;
	}

	AttackType ResolveAttackType(const Tga::StringId anAttackTypeName)
	{
		if (MatchesText(anAttackTypeName, "MeleeCombo")) return AttackType::MeleeCombo;
		if (MatchesText(anAttackTypeName, "MeleeCharged")) return AttackType::MeleeCharged;
		if (MatchesText(anAttackTypeName, "DodgeAttack")) return AttackType::DodgeAttack;
		if (MatchesText(anAttackTypeName, "Ranged")) return AttackType::Ranged;
		if (MatchesText(anAttackTypeName, "EnemyMelee")) return AttackType::EnemyMelee;
		if (MatchesText(anAttackTypeName, "EnemyRoll")) return AttackType::EnemyRoll;
		return AttackType::MeleeLight;
	}

	CollisionShapeType ResolveCollisionShape(const Tga::StringId aShapeName)
	{
		if (MatchesText(aShapeName, "Box"))
		{
			return CollisionShapeType::Box;
		}

		return CollisionShapeType::Sphere;
	}

	ObjectLayer ResolveObjectLayer(const Tga::StringId aLayerName, const CombatTeam aTeam)
	{
		if (aLayerName.IsEmpty() || MatchesText(aLayerName, "Opposing"))
		{
			return aTeam == CombatTeam::Enemy ? ObjectLayer::Player : ObjectLayer::Enemy;
		}

		if (MatchesText(aLayerName, "Player")) return ObjectLayer::Player;
		if (MatchesText(aLayerName, "Enemy")) return ObjectLayer::Enemy;
		if (MatchesText(aLayerName, "WorldStatic")) return ObjectLayer::WorldStatic;
		if (MatchesText(aLayerName, "Projectile")) return ObjectLayer::Projectile;
		if (MatchesText(aLayerName, "Trigger")) return ObjectLayer::Trigger;
		if (MatchesText(aLayerName, "Pickup")) return ObjectLayer::Pickup;
		if (MatchesText(aLayerName, "NPC")) return ObjectLayer::NPC;
		if (MatchesText(aLayerName, "Switch")) return ObjectLayer::Switch;

		return aTeam == CombatTeam::Enemy ? ObjectLayer::Player : ObjectLayer::Enemy;
	}
}

void AnimationEventScriptUpdateContext::PlaySfx(Tga::StringId aSoundName)
{
	SoundID soundId = SoundID::eUnknown;
	if (Essentials::globalAudioManager && TryResolveSoundId(aSoundName, soundId))
	{
		Essentials::globalAudioManager->PlaySFX(soundId);
	}
	else
	{
		std::cout << "[AnimationEventScript] Unknown sound '" << aSoundName.GetString() << "'\n";
	}
}

void AnimationEventScriptUpdateContext::StartCombatAttack(const Tga::AnimationEventCombatAttackDesc& anAttack)
{
	if (!event || !event->owner)
	{
		return;
	}

	AttackData attack;
	attack.owner = event->owner;
	attack.team = ResolveCombatTeam(anAttack.team, *event->owner);
	attack.type = ResolveAttackType(anAttack.attackType);
	attack.collisionShape = ResolveCollisionShape(anAttack.collisionShape);
	attack.localCenterOffset = CommonUtilities::Vector3<float>(
		anAttack.localOffsetX,
		anAttack.localOffsetY,
		anAttack.localOffsetZ);
	attack.size = CommonUtilities::Vector3<float>(
		(std::max)(0.0f, anAttack.sizeX),
		(std::max)(0.0f, anAttack.sizeY),
		(std::max)(0.0f, anAttack.sizeZ));
	attack.radius = (std::max)(0.0f, anAttack.radius);
	attack.activeDurationSeconds = (std::max)(0.0f, anAttack.activeDurationSeconds);
	attack.knockbackStrength = (std::max)(0.0f, anAttack.knockbackStrength);
	attack.damage = (std::max)(0, anAttack.damage);
	attack.onlyHitForwardHemisphere = anAttack.onlyHitForwardHemisphere;
	attack.targetLayers.AddLayer(ResolveObjectLayer(anAttack.targetLayer, attack.team));

	if (attack.collisionShape == CollisionShapeType::Box &&
		attack.size.x <= 0.0f &&
		attack.size.y <= 0.0f &&
		attack.size.z <= 0.0f)
	{
		const float fallbackExtent = attack.radius > 0.0f ? attack.radius * 2.0f : 100.0f;
		attack.size = CommonUtilities::Vector3<float>(fallbackExtent, fallbackExtent, fallbackExtent);
	}

	if (CombatService::StartAttack(attack) == 0)
	{
		std::cout << "[AnimationEventScript] Failed to start combat attack for '"
			<< event->owner->GetName() << "'\n";
	}
}

void AnimationEventScriptUpdateContext::FirePlayerProjectile()
{
	if (!event || !event->owner)
	{
		return;
	}

	PlayerControllerComponent* player = event->owner->GetComponent<PlayerControllerComponent>();
	if (!player)
	{
		std::cout << "[AnimationEventScript] Fire Player Projectile requires a PlayerControllerComponent on '"
			<< event->owner->GetName() << "'\n";
		return;
	}

	player->FireBullet();
}

void AnimationEventScriptUpdateContext::TriggerCameraShake(float aDurationSeconds, float anIntensityUnits)
{
	if (Essentials::globalCamera)
	{
		Essentials::globalCamera->TriggerCameraShake(aDurationSeconds, anIntensityUnits);
	}
}

void AnimationEventScriptUpdateContext::LogAnimationEvent() const
{
	if (event)
	{
		Tga::AnimationEventScriptContext::LogAnimationEvent();
	}
}

void AnimationEventService::Dispatch(const AnimationEventContext& anEvent)
{
	if (anEvent.record.scriptId.IsEmpty())
	{
		return;
	}

	RuntimeScript* runtimeScript = GetOrCreateRuntimeScript(anEvent);
	if (!runtimeScript)
	{
		return;
	}

	runtimeScript->lastEvent = anEvent;
	runtimeScript->hasLastEvent = true;

	if (runtimeScript->eventTriggerPins.empty())
	{
		LogMissingTriggerOnce(runtimeScript->scriptId);
		return;
	}

	AnimationEventScriptUpdateContext context = MakeContext(*runtimeScript, 0.0f, true);
	for (const Tga::ScriptPinId pin : runtimeScript->eventTriggerPins)
	{
		runtimeScript->instance->TriggerPin(pin, context);
	}
}

void AnimationEventService::Update(float aDeltaTime)
{
	for (std::unique_ptr<RuntimeScript>& runtimeScript : myRuntimeScripts)
	{
		if (!runtimeScript || !runtimeScript->instance || !runtimeScript->hasLastEvent)
		{
			continue;
		}

		AnimationEventScriptUpdateContext context = MakeContext(*runtimeScript, aDeltaTime, false);
		runtimeScript->instance->Update(context);
	}
}

void AnimationEventService::ReleaseOwner(GameObject* anOwner)
{
	if (!anOwner)
	{
		return;
	}

	myRuntimeScripts.erase(
		std::remove_if(
			myRuntimeScripts.begin(),
			myRuntimeScripts.end(),
			[anOwner](const std::unique_ptr<RuntimeScript>& aRuntimeScript)
			{
				return !aRuntimeScript || aRuntimeScript->owner == anOwner;
			}),
		myRuntimeScripts.end());
}

void AnimationEventService::Clear()
{
	myRuntimeScripts.clear();
	myMissingScriptWarnings.clear();
	myMissingTriggerWarnings.clear();
}

AnimationEventService::RuntimeScript* AnimationEventService::GetOrCreateRuntimeScript(const AnimationEventContext& anEvent)
{
	if (!anEvent.owner || anEvent.record.scriptId.IsEmpty())
	{
		return nullptr;
	}

	for (std::unique_ptr<RuntimeScript>& runtimeScript : myRuntimeScripts)
	{
		if (runtimeScript &&
			runtimeScript->owner == anEvent.owner &&
			runtimeScript->scriptId == anEvent.record.scriptId)
		{
			runtimeScript->graph = anEvent.graph;
			return runtimeScript.get();
		}
	}

	std::shared_ptr<const Tga::Script> script = Tga::ScriptManager::GetScript(anEvent.record.scriptId.GetString());
	if (!script)
	{
		LogMissingScriptOnce(anEvent.record.scriptId);
		return nullptr;
	}

	std::unique_ptr<RuntimeScript> runtimeScript = std::make_unique<RuntimeScript>();
	runtimeScript->owner = anEvent.owner;
	runtimeScript->graph = anEvent.graph;
	runtimeScript->scriptId = anEvent.record.scriptId;
	runtimeScript->script = script;
	runtimeScript->instance = std::make_unique<Tga::ScriptRuntimeInstance>(script);
	runtimeScript->instance->Init();
	CacheEventTriggerPins(*runtimeScript);

	RuntimeScript* result = runtimeScript.get();
	myRuntimeScripts.push_back(std::move(runtimeScript));
	return result;
}

void AnimationEventService::CacheEventTriggerPins(RuntimeScript& aRuntimeScript)
{
	const Tga::ScriptNodeTypeId eventNodeType = Tga::ScriptNodeTypeRegistry::GetTypeId("On Animation Event");
	if (eventNodeType.id == Tga::ScriptNodeTypeId::InvalidId)
	{
		return;
	}

	const Tga::Script& script = *aRuntimeScript.script;
	for (Tga::ScriptNodeId nodeId = script.GetFirstNodeId();
		nodeId.id != Tga::ScriptNodeId::InvalidId;
		nodeId = script.GetNextNodeId(nodeId))
	{
		if (script.GetType(nodeId) != eventNodeType)
		{
			continue;
		}

		size_t pinCount = 0;
		const Tga::ScriptPinId* pins = script.GetInputPins(nodeId, pinCount);
		for (size_t pinIndex = 0; pinIndex < pinCount; ++pinIndex)
		{
			const Tga::ScriptPin& pin = script.GetPin(pins[pinIndex]);
			if (pin.type == Tga::ScriptLinkType::Flow)
			{
				aRuntimeScript.eventTriggerPins.push_back(pins[pinIndex]);
				break;
			}
		}
	}
}

AnimationEventScriptUpdateContext AnimationEventService::MakeContext(
	RuntimeScript& aRuntimeScript,
	float aDeltaTime,
	bool aIsDispatchingEvent)
{
	AnimationEventScriptUpdateContext context;
	context.dynamicProperties = &aRuntimeScript.dynamicProperties;
	context.staticProperties = nullptr;
	context.deltaTime = aDeltaTime;
	context.frameNumber = Tga::Engine::GetInstance()
		? static_cast<int>(Tga::Engine::GetInstance()->GetTotalTime() * 1000.0f)
		: 0;
	context.event = aRuntimeScript.hasLastEvent ? &aRuntimeScript.lastEvent : nullptr;
	context.isDispatchingEvent = aIsDispatchingEvent;

	if (context.event)
	{
		context.eventId = context.event->record.id;
		context.clipPath = context.event->record.clipPath;
		context.scriptId = context.event->record.scriptId;
		context.eventTime = context.event->record.time;
	}

	return context;
}

void AnimationEventService::LogMissingScriptOnce(Tga::StringId aScriptId)
{
	if (std::find(myMissingScriptWarnings.begin(), myMissingScriptWarnings.end(), aScriptId) != myMissingScriptWarnings.end())
	{
		return;
	}

	myMissingScriptWarnings.push_back(aScriptId);
	std::cout << "[AnimationEventScript] Missing script '" << aScriptId.GetString() << "'\n";
}

void AnimationEventService::LogMissingTriggerOnce(Tga::StringId aScriptId)
{
	if (std::find(myMissingTriggerWarnings.begin(), myMissingTriggerWarnings.end(), aScriptId) != myMissingTriggerWarnings.end())
	{
		return;
	}

	myMissingTriggerWarnings.push_back(aScriptId);
	std::cout << "[AnimationEventScript] Script '" << aScriptId.GetString()
		<< "' has no On Animation Event node\n";
}
