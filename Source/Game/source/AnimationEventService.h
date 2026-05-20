#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <tge/script/ScriptCommon.h>
#include <tge/script/Contexts/AnimationEventScriptContext.h>
#include <tge/script/ScriptRuntimeInstance.h>
#include <tge/stringRegistry/StringRegistry.h>

#include "AnimationEventListener.h"

class AnimationGraphComponent;
class GameObject;

namespace Tga
{
    class Script;
}

struct AnimationEventScriptUpdateContext final : public Tga::AnimationEventScriptContext
{
    const AnimationEventContext* event = nullptr;

    void PlaySfx(Tga::StringId aSoundName) override;
    void StartCombatAttack(const Tga::AnimationEventCombatAttackDesc& anAttack) override;
    void FirePlayerProjectile() override;
    void TriggerCameraShake(float aDurationSeconds, float anIntensityUnits) override;
    void LogAnimationEvent() const override;
};

class AnimationEventService final
{
public:
    void Dispatch(const AnimationEventContext& anEvent);
    void Update(float aDeltaTime);
    void ReleaseOwner(GameObject* anOwner);
    void Clear();

private:
    struct RuntimeScript
    {
        GameObject* owner = nullptr;
        AnimationGraphComponent* graph = nullptr;
        Tga::StringId scriptId;
        std::shared_ptr<const Tga::Script> script;
        std::unique_ptr<Tga::ScriptRuntimeInstance> instance;
        std::vector<Tga::ScriptPinId> eventTriggerPins;
        AnimationEventContext lastEvent;
        bool hasLastEvent = false;
        std::unordered_map<Tga::StringId, Tga::Property> dynamicProperties;
    };

    RuntimeScript* GetOrCreateRuntimeScript(const AnimationEventContext& anEvent);
    void CacheEventTriggerPins(RuntimeScript& aRuntimeScript);
    AnimationEventScriptUpdateContext MakeContext(RuntimeScript& aRuntimeScript, float aDeltaTime, bool aIsDispatchingEvent);
    void LogMissingScriptOnce(Tga::StringId aScriptId);
    void LogMissingTriggerOnce(Tga::StringId aScriptId);

    std::vector<std::unique_ptr<RuntimeScript>> myRuntimeScripts;
    std::vector<Tga::StringId> myMissingScriptWarnings;
    std::vector<Tga::StringId> myMissingTriggerWarnings;
};
