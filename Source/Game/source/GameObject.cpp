#include "GameObject.h"
#include "ToggleComponent.h"
#include "BoxColliderComponent.h"

#include <tge/debug/LoadingProfiler.h>

#include <atomic>
#include <chrono>
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
    const auto startTime = std::chrono::steady_clock::now();

    myEngine = &anEngine;
    myIsInitialized = true;

    for (auto& component : myComponents)
    {
        component->Init(anEngine);
    }

    if (ToggleComponent* toggleComponent = GetComponent<ToggleComponent>())
    {
        GetComponent<ToggleComponent>()->Initialize();
        /*auto& transform = GetTransform();
        this->GetTransform().SetPosition(Tga::Vector3f(transform.GetPosition().x, transform.GetPosition().y - 400.0f, transform.GetPosition().z));*/
    }
    if (this->HasComponent<BoxColliderComponent>())
    {
        GetComponent<BoxColliderComponent>()->Init(anEngine);
    }

    const double milliseconds =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTime).count();
    Tga::LoadingProfiler::GetInstance().RecordGameObjectInit(myName, myTag, milliseconds);
}

void GameObject::Reset()
{
    for (auto& component : myComponents)
    {
        component->Reset();
    }
}

void GameObject::Save()
{
    for (auto& component : myComponents)
    {
        component->Save();
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

void GameObject::SetObjDefinition(const std::string& aObjDef)
{
    myObjDefinition = aObjDef;
}

const std::string& GameObject::GetObjDefinition() const
{
    return myObjDefinition;
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

void GameObject::DisableAllComponents()
{
    for (auto& component : myComponents)
    {
        component->SetEnabled(false);
    }
}
