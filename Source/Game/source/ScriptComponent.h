#pragma once

#include "Component.h"

class ScriptComponent : public Component
{
public:
    void Init(Tga::Engine& anEngine) override;
    void FixedUpdate(float aFixedDeltaTime) override final;
    void Update(float aDeltaTime) override final;
    void LateUpdate(float aDeltaTime) override final;
    void OnDestroy() override final;
    void OnActiveChanged(bool isActive) override final;
    void OnEnabledChanged(bool isEnabled) override final;

protected:
    virtual void OnStart() {}
    virtual void OnEnable() {}
    virtual void OnUpdate(float /*aDeltaTime*/) {}
    virtual void OnLateUpdate(float /*aDeltaTime*/) {}
    virtual void OnFixedUpdate(float /*aFixedDeltaTime*/) {}
    virtual void OnDisable() {}
    virtual void OnScriptDestroy() {}

    bool HasStarted() const { return myHasStarted; }

private:
    void EnsureStarted();
    bool IsActiveAndEnabledInHierarchy() const;
    void RefreshRuntimeActivation();

    bool myHasStarted = false;
    bool myIsRuntimeActive = false;
};
