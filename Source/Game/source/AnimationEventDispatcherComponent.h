#pragma once

#include "ScriptComponent.h"

struct AnimationEventContext;
class AnimationGraphComponent;

class AnimationEventDispatcherComponent final : public ScriptComponent
{
protected:
    void OnStart() override;
    void OnEnable() override;
    void OnLateUpdate(float aDeltaTime) override;
    void OnDisable() override;
    void OnScriptDestroy() override;

private:
    AnimationGraphComponent* ResolveGraph();
    void ClearQueuedEvents();
    void DispatchToLocalListeners(const AnimationEventContext& anEvent) const;
    void DispatchToPostMaster(const AnimationEventContext& anEvent) const;

    AnimationGraphComponent* myGraph = nullptr;
};
