#pragma once

namespace Tga
{
    class Engine;
}

class GameObject;

/// Base class for all components attached to a GameObject.
class Component
{
public:
    virtual ~Component() = default;

    virtual void Init(Tga::Engine& /*anEngine*/) {}
    virtual void FixedUpdate(float /*aFixedDeltaTime*/) {}
    virtual void Update(float /*aDeltaTime*/) {}
    virtual void LateUpdate(float /*aDeltaTime*/) {}
    virtual void Render() {}
    virtual void OnDestroy() {}
    virtual void OnActiveChanged(bool /*isActive*/) {}
    virtual void OnEnabledChanged(bool /*isEnabled*/) {}

    bool IsEnabled() const { return myIsEnabled; }
    void SetEnabled(bool anEnabled)
    {
        if (myIsEnabled == anEnabled)
        {
            return;
        }

        myIsEnabled = anEnabled;
        OnEnabledChanged(myIsEnabled);
    }

    GameObject* GetOwner() const { return myOwner; }

private:
    void SetOwner(GameObject* anOwner) { myOwner = anOwner; }

    GameObject* myOwner = nullptr;
    bool myIsEnabled = true;

    friend class GameObject;
};
