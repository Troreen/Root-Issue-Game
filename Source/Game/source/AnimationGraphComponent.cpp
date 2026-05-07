#include "AnimationGraphComponent.h"

#include "AnimatedMeshComponent.h"
#include "GameObject.h"

#include <utility>

namespace
{
	constexpr float RootMotionEpsilon = 1e-6f;

    bool TryReadStringFromAny(const std::any& aValue, std::string& outValue)
    {
        if (const auto* asString = std::any_cast<std::string>(&aValue))
        {
            outValue = *asString;
            return true;
        }

        if (const auto* asStringId = std::any_cast<Tga::StringId>(&aValue))
        {
            outValue = asStringId->GetString();
            return true;
        }

        return false;
    }

    CommonUtilities::Vector3<float> ToCommonVector(const Tga::Vector3f& aValue)
    {
        return CommonUtilities::Vector3<float>(aValue.X, aValue.Y, aValue.Z);
    }

    CommonUtilities::Quaternion<float> ToCommonQuaternion(const Tga::Quatf& aValue)
    {
        return CommonUtilities::Quaternion<float>(aValue.W, aValue.X, aValue.Y, aValue.Z);
    }

    CommonUtilities::Vector3<float> RotateLocalOffsetToWorld(
        const Transformf& aTransform,
        const CommonUtilities::Vector3<float>& aLocalOffset)
    {
        return aTransform.GetRight() * aLocalOffset.x
            + aTransform.GetUp() * aLocalOffset.y
            + aTransform.GetForward() * aLocalOffset.z;
    }
}

AnimationGraphComponent::AnimationGraphComponent(std::string aGraphPath)
    : myGraphPath(std::move(aGraphPath))
{
}

void AnimationGraphComponent::SetGraphPath(const std::string& aGraphPath)
{
    myGraphPath = aGraphPath;
}

const std::string& AnimationGraphComponent::GetGraphPath() const
{
    return myGraphPath;
}

void AnimationGraphComponent::SetModelPropertyName(const std::string& aModelPropertyName)
{
    if (!aModelPropertyName.empty())
    {
        myModelPropertyName = aModelPropertyName;
    }
}

void AnimationGraphComponent::SetApplyRootMotion(const bool aShouldApplyRootMotion)
{
    myApplyRootMotion = aShouldApplyRootMotion;
}

bool AnimationGraphComponent::GetApplyRootMotion() const
{
    return myApplyRootMotion;
}

void AnimationGraphComponent::SetApplyRootMotionRotation(const bool aShouldApplyRootMotionRotation)
{
    myApplyRootMotionRotation = aShouldApplyRootMotionRotation;
}

bool AnimationGraphComponent::GetApplyRootMotionRotation() const
{
    return myApplyRootMotionRotation;
}

void AnimationGraphComponent::SetRootMotionTranslationScale(const float aScale)
{
    myRootMotionTranslationScale = (std::max)(0.0f, aScale);
}

float AnimationGraphComponent::GetRootMotionTranslationScale() const
{
    return myRootMotionTranslationScale;
}

void AnimationGraphComponent::SetSourceProperties(const std::unordered_map<std::string, std::any>& someProperties)
{
    mySourceProperties = someProperties;
}

bool AnimationGraphComponent::SetFloatParameter(const std::string& aName, const float aValue)
{
    return myRuntime.SetFloatParameter(aName, aValue);
}

bool AnimationGraphComponent::SetIntParameter(const std::string& aName, const int aValue)
{
    return myRuntime.SetIntParameter(aName, aValue);
}

bool AnimationGraphComponent::SetBoolParameter(const std::string& aName, const bool aValue)
{
    return myRuntime.SetBoolParameter(aName, aValue);
}

bool AnimationGraphComponent::SetVector3Parameter(const std::string& aName, const Tga::Vector3f& aValue)
{
    return myRuntime.SetVector3Parameter(aName, aValue);
}

bool AnimationGraphComponent::SetStringParameter(const std::string& aName, const std::string& aValue)
{
    return myRuntime.SetStringParameter(aName, aValue);
}

AnimationEventQueue& AnimationGraphComponent::GetEventQueue()
{
    return myEventQueue;
}

const AnimationEventQueue& AnimationGraphComponent::GetEventQueue() const
{
    return myEventQueue;
}

bool AnimationGraphComponent::ConsumeRootMotion(Tga::Vector3f& outTranslation, Tga::Quatf& outRotation)
{
    if (!myRuntime.IsReady())
    {
        return false;
    }

    outTranslation = myPendingRootMotionTranslation;
    outRotation = myPendingRootMotionRotation;

    myPendingRootMotionTranslation = Tga::Vector3f();
    myPendingRootMotionRotation = Tga::Quatf();
    return true;
}

void AnimationGraphComponent::Reset()
{
    myRuntime.Reset();
    myEventQueue.Clear();
}

void AnimationGraphComponent::OnStart()
{
    InitializeRuntimeIfPossible();
}

void AnimationGraphComponent::OnUpdate(const float aDeltaTime)
{
    if (!myAnimatedMesh)
    {
        if (GameObject* owner = GetOwner())
        {
            myAnimatedMesh = owner->GetComponent<AnimatedMeshComponent>();
        }
    }

    if (!myAnimatedMesh || !myAnimatedMesh->IsValid())
    {
        return;
    }

    if (!myRuntime.IsReady())
    {
        if (!InitializeRuntimeIfPossible())
        {
            return;
        }
    }

    Tga::LocalSpacePose generatedPose;
    if (myRuntime.Update(aDeltaTime, myAnimatedMesh->GetModel(), generatedPose))
    {
        myAnimatedMesh->SetPose(generatedPose);

        const Tga::Vector3f rootMotionTranslationDelta = myRuntime.ConsumeRootMotionTranslation();
        const Tga::Quatf rootMotionRotationDelta = myRuntime.ConsumeRootMotionRotation();

        AppendPendingRootMotion(rootMotionTranslationDelta, rootMotionRotationDelta);

        if (myApplyRootMotion)
        {
            ApplyRootMotionToOwner(rootMotionTranslationDelta, rootMotionRotationDelta);
        }
    }
}

void AnimationGraphComponent::OnDisable()
{
    myEventQueue.Clear();
    myPendingRootMotionTranslation = Tga::Vector3f();
    myPendingRootMotionRotation = Tga::Quatf();
}

void AnimationGraphComponent::OnScriptDestroy()
{
    myRuntime.Reset();
    myEventQueue.Clear();
    myAnimatedMesh = nullptr;
	myPendingRootMotionTranslation = Tga::Vector3f();
	myPendingRootMotionRotation = Tga::Quatf();
}

bool AnimationGraphComponent::InitializeRuntimeIfPossible()
{
    GameObject* owner = GetOwner();
    if (!owner)
    {
        return false;
    }

    if (!myAnimatedMesh)
    {
        myAnimatedMesh = owner->GetComponent<AnimatedMeshComponent>();
    }

    if (!myAnimatedMesh)
    {
        return false;
    }

    if (myGraphPath.empty())
    {
        auto it = mySourceProperties.find("animation_graph");
        if (it != mySourceProperties.end())
        {
            TryReadStringFromAny(it->second, myGraphPath);
        }

        if (myGraphPath.empty())
        {
            it = mySourceProperties.find("animationGraph");
            if (it != mySourceProperties.end())
            {
                TryReadStringFromAny(it->second, myGraphPath);
            }
        }
    }

    if (myGraphPath.empty())
    {
        return false;
    }

    const bool wasInitialized = myRuntime.Initialize(
        myGraphPath,
        myModelPropertyName,
        mySourceProperties,
        myAnimatedMesh->GetModelPath(),
        &myEventQueue);

    if (wasInitialized)
    {
        myPendingRootMotionTranslation = Tga::Vector3f();
        myPendingRootMotionRotation = Tga::Quatf();
    }

    return wasInitialized;
}

void AnimationGraphComponent::AppendPendingRootMotion(
    const Tga::Vector3f& aTranslationDelta,
    const Tga::Quatf& aRotationDelta)
{
    myPendingRootMotionTranslation += aTranslationDelta;
    myPendingRootMotionRotation *= aRotationDelta;
    myPendingRootMotionRotation.Normalize();
}

void AnimationGraphComponent::ApplyRootMotionToOwner(
    const Tga::Vector3f& aTranslationDelta,
    const Tga::Quatf& aRotationDelta)
{
    GameObject* owner = GetOwner();
    if (!owner)
    {
        return;
    }

    Transformf& ownerTransform = owner->GetTransform();

    CommonUtilities::Vector3<float> localTranslationDelta = ToCommonVector(aTranslationDelta);
    localTranslationDelta *= myRootMotionTranslationScale;

    if (localTranslationDelta.LengthSqr() > RootMotionEpsilon)
    {
        const CommonUtilities::Vector3<float> worldTranslationDelta =
            RotateLocalOffsetToWorld(ownerTransform, localTranslationDelta);
        ownerTransform.Translate(worldTranslationDelta);
    }

    if (myApplyRootMotionRotation)
    {
        CommonUtilities::Quaternion<float> rotationDelta = ToCommonQuaternion(aRotationDelta).GetNormalized();

        CommonUtilities::Quaternion<float> newRotation = ownerTransform.GetRotation() * rotationDelta;
        newRotation.Normalize();
        ownerTransform.SetRotation(newRotation);
    }
}
