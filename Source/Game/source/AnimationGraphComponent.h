#pragma once

#include "AnimationEventQueue.h"
#include "AnimationGraphRuntime.h"
#include "ScriptComponent.h"

#include <any>
#include <string>
#include <unordered_map>

class AnimatedMeshComponent;

// Game-facing wrapper around AnimationGraphRuntime.
// Owns the graph instance for one GameObject, applies generated poses to the
// sibling AnimatedMeshComponent, and optionally applies root motion to the owner transform.
class AnimationGraphComponent final : public ScriptComponent
{
public:
    AnimationGraphComponent() = default;
    explicit AnimationGraphComponent(std::string aGraphPath);

    void SetGraphPath(const std::string& aGraphPath);
    const std::string& GetGraphPath() const;

    void SetModelPropertyName(const std::string& aModelPropertyName);

    void SetApplyRootMotion(bool aShouldApplyRootMotion);
    bool GetApplyRootMotion() const;

    void SetApplyRootMotionRotation(bool aShouldApplyRootMotionRotation);
    bool GetApplyRootMotionRotation() const;

    void SetRootMotionTranslationScale(float aScale);
    float GetRootMotionTranslationScale() const;

    void SetSourceProperties(const std::unordered_map<std::string, std::any>& someProperties);

    // Parameter writes are intentionally thin pass-throughs so gameplay code does not
    // need to know about TGE script Property types or StringId conversion.
    bool SetFloatParameter(const std::string& aName, float aValue);
    bool SetIntParameter(const std::string& aName, int aValue);
    bool SetBoolParameter(const std::string& aName, bool aValue);
    bool SetVector3Parameter(const std::string& aName, const Tga::Vector3f& aValue);
    bool SetStringParameter(const std::string& aName, const std::string& aValue);

    AnimationEventQueue& GetEventQueue();
    const AnimationEventQueue& GetEventQueue() const;

    // Returns root motion accumulated since the previous consume without applying it.
    // This is useful for controllers that want to own movement themselves.
    bool ConsumeRootMotion(Tga::Vector3f& outTranslation, Tga::Quatf& outRotation);

    void Reset() override;

protected:
    void OnStart() override;
    void OnUpdate(float aDeltaTime) override;
    void OnDisable() override;
    void OnScriptDestroy() override;

private:
    bool InitializeRuntimeIfPossible();
    void AppendPendingRootMotion(const Tga::Vector3f& aTranslationDelta, const Tga::Quatf& aRotationDelta);
    void ApplyRootMotionToOwner(const Tga::Vector3f& aTranslationDelta, const Tga::Quatf& aRotationDelta);

    std::string myGraphPath;
    std::string myModelPropertyName = "model";

    bool myApplyRootMotion = false;
    bool myApplyRootMotionRotation = false;
    float myRootMotionTranslationScale = 1.0f;

    std::unordered_map<std::string, std::any> mySourceProperties;

    // Cached after startup because graph evaluation runs every frame.
    AnimatedMeshComponent* myAnimatedMesh = nullptr;

    AnimationEventQueue myEventQueue;
    AnimationGraphRuntime myRuntime;

    Tga::Vector3f myPendingRootMotionTranslation = Tga::Vector3f();
    Tga::Quatf myPendingRootMotionRotation = Tga::Quatf();
};
