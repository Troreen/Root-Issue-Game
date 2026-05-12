#pragma once

#include "ScriptComponent.h"

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

    AnimationGraphComponent* myGraph = nullptr;
};
