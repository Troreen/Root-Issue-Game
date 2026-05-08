#pragma once

#include <CommonUtilities/Transform.hpp>
#include "Component.h"
#include "ObjectLayer.h"
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>


namespace Tga
{
    class Engine;
}

using Vector3f = CommonUtilities::Vector3<float>;
using Transformf = CommonUtilities::Transform<float>;

/// Base class for all game objects in the game world.
/// Provides common functionality like position, rotation, input handling, and rendering.
class GameObject
{
public:
    explicit GameObject(std::string aName = "GameObject");
    virtual ~GameObject();

    /// Initialize the game object. Called once when the object is created.
    /// Override this to load textures, set up sprites, etc.
    virtual void Init(Tga::Engine& anEngine);

    //Reloads data in componets. Called when ReloadComponent recieves a message
    virtual void Reset();

    /// Update the game object. Called every frame.
    /// <param name="aDeltaTime">Time in seconds since last frame</param>
    virtual void Update(float aDeltaTime);

    /// Fixed update pass. Called at a fixed timestep for deterministic logic.
    virtual void FixedUpdate(float aFixedDeltaTime);

    /// Late update pass. Called after regular Update for ordering-sensitive logic.
    virtual void LateUpdate(float aDeltaTime);

    /// Render the game object. Called every frame after Update.
    virtual void Render();

    Transformf& GetTransform();
    const Transformf& GetTransform() const;

    void SetName(const std::string& aName);
    const std::string& GetName() const;

    void SetObjDefinition(const std::string& aObjDef);
    const std::string& GetObjDefinition() const;

    void SetTag(const std::string& aTag);
    const std::string& GetTag() const;

    void SetActive(bool anIsActive);
    bool IsActive() const;

    void SetPersistent(bool anIsPersistent);
    bool IsPersistent() const;

    void SetLayer(ObjectLayer aLayer);
    ObjectLayer GetLayer() const;

    std::uint64_t GetCollisionId() const;

    template <typename T, typename... Args>
    T* AddComponent(Args&&... someArgs)
    {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

        auto component = std::make_unique<T>(std::forward<Args>(someArgs)...);
        T* rawPtr = component.get();
        rawPtr->SetOwner(this);

        myComponents.push_back(std::move(component));

        if (myIsInitialized && myEngine)
        {
            rawPtr->Init(*myEngine);
        }

        return rawPtr;
    }

    template <typename T>
    T* GetComponent() const
    {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

        for (const auto& component : myComponents)
        {
            if (auto* casted = dynamic_cast<T*>(component.get()))
            {
                return casted;
            }
        }

        return nullptr;
    }

    template <typename T>
    bool HasComponent() const
    {
        return GetComponent<T>() != nullptr;
    }

    template <typename T>
    bool RemoveComponent()
    {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

        for (auto it = myComponents.begin(); it != myComponents.end(); ++it)
        {
            if (dynamic_cast<T*>(it->get()))
            {
                (*it)->OnDestroy();
                myComponents.erase(it);
                return true;
            }
        }

        return false;
    }

    void RemoveAllComponents();

protected:
    Transformf myTransform;

private:
    std::string myName;
    std::string myObjDefinition;
    std::string myTag;
    bool myIsActive = true;
    bool myIsPersistent = false;
    bool myIsInitialized = false;
    Tga::Engine* myEngine = nullptr;
    ObjectLayer myLayer = ObjectLayer::WorldStatic;
    std::uint64_t myCollisionId = 0;

    std::vector<std::unique_ptr<Component>> myComponents;
};
