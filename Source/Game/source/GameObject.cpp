#include "GameObject.h"

#include <atomic>
#include <cstdint>
#include <utility>

namespace
{
    std::atomic<std::uint64_t> gNextCollisionId = 1;
}

GameObject::GameObject(std::string aName)
    : myTransform()
    , myName(std::move(aName))
    , myCollisionId(gNextCollisionId.fetch_add(1, std::memory_order_relaxed))
{
}

GameObject::~GameObject()
{
    RemoveAllComponents();
}

void GameObject::Init(Tga::Engine& anEngine)
{
    myEngine = &anEngine;
    myIsInitialized = true;

    for (auto& component : myComponents)
    {
        component->Init(anEngine);
    }
}

void GameObject::Update(float aDeltaTime)
{
    if (!myIsActive)
    {
        return;
    }

    for (auto& component : myComponents)
    {
        if (component->IsEnabled())
        {
            component->Update(aDeltaTime);
        }
    }
}

void GameObject::FixedUpdate(float aFixedDeltaTime)
{
    if (!myIsActive)
    {
        return;
    }

    for (auto& component : myComponents)
    {
        if (component->IsEnabled())
        {
            component->FixedUpdate(aFixedDeltaTime);
        }
    }
}

void GameObject::LateUpdate(float aDeltaTime)
{
    if (!myIsActive)
    {
        return;
    }

    for (auto& component : myComponents)
    {
        if (component->IsEnabled())
        {
            component->LateUpdate(aDeltaTime);
        }
    }
}

Transformf& GameObject::GetTransform()
{
    return myTransform;
}

const Transformf& GameObject::GetTransform() const
{
    return myTransform;
}

void GameObject::Render()
{
    if (!myIsActive)
    {
        return;
    }

    for (auto& component : myComponents)
    {
        if (component->IsEnabled())
        {
            component->Render();
        }
    }
}

void GameObject::SetName(const std::string& aName)
{
    myName = aName;
}

const std::string& GameObject::GetName() const
{
    return myName;
}

void GameObject::SetTag(const std::string& aTag)
{
    myTag = aTag;
}

const std::string& GameObject::GetTag() const
{
    return myTag;
}

void GameObject::SetActive(bool anIsActive)
{
    if (myIsActive == anIsActive)
    {
        return;
    }

    myIsActive = anIsActive;

    for (auto& component : myComponents)
    {
        component->OnActiveChanged(myIsActive);
    }
}

bool GameObject::IsActive() const
{
    return myIsActive;
}

void GameObject::SetPersistent(bool anIsPersistent)
{
    myIsPersistent = anIsPersistent;
}

bool GameObject::IsPersistent() const
{
    return myIsPersistent;
}

void GameObject::SetLayer(ObjectLayer aLayer)
{
    myLayer = aLayer;
}

ObjectLayer GameObject::GetLayer() const
{
    return myLayer;
}

std::uint64_t GameObject::GetCollisionId() const
{
    return myCollisionId;
}

void GameObject::RemoveAllComponents()
{
    for (auto it = myComponents.rbegin(); it != myComponents.rend(); ++it)
    {
        (*it)->OnDestroy();
    }

    myComponents.clear();
}
