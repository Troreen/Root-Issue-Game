#include "ScriptComponent.h"

#include "GameObject.h"

void ScriptComponent::Init(Tga::Engine& anEngine)
{
    Component::Init(anEngine);
    RefreshRuntimeActivation();
}

void ScriptComponent::FixedUpdate(float aFixedDeltaTime)
{
    RefreshRuntimeActivation();
    EnsureStarted();
    OnFixedUpdate(aFixedDeltaTime);
}

void ScriptComponent::Update(float aDeltaTime)
{
    RefreshRuntimeActivation();
    EnsureStarted();
    OnUpdate(aDeltaTime);
}

void ScriptComponent::LateUpdate(float aDeltaTime)
{
    RefreshRuntimeActivation();
    EnsureStarted();
    OnLateUpdate(aDeltaTime);
}

void ScriptComponent::OnDestroy()
{
    if (myIsRuntimeActive || IsActiveAndEnabledInHierarchy())
    {
        myIsRuntimeActive = false;
        OnDisable();
    }

    OnScriptDestroy();
}

void ScriptComponent::OnActiveChanged(bool /*isActive*/)
{
    RefreshRuntimeActivation();
}

void ScriptComponent::OnEnabledChanged(bool /*isEnabled*/)
{
    RefreshRuntimeActivation();
}

void ScriptComponent::EnsureStarted()
{
    if (myHasStarted)
    {
        return;
    }

    myHasStarted = true;
    OnStart();
}

bool ScriptComponent::IsActiveAndEnabledInHierarchy() const
{
    const GameObject* owner = GetOwner();
    if (!owner)
    {
        return false;
    }

    return owner->IsActive() && IsEnabled();
}

void ScriptComponent::RefreshRuntimeActivation()
{
    const bool shouldBeRuntimeActive = IsActiveAndEnabledInHierarchy();
    if (shouldBeRuntimeActive == myIsRuntimeActive)
    {
        return;
    }

    myIsRuntimeActive = shouldBeRuntimeActive;

    if (myIsRuntimeActive)
    {
        OnEnable();
    }
    else
    {
        OnDisable();
    }
}
