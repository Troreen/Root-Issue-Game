#include "AnimationGraphRuntime.h"

#include "AnimationEventQueue.h"

#include <CommonUtilities/Vector3.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <vector>

#include <tge/animation/PoseGenerator.h>
#include <tge/math/Color.h>
#include <tge/scene/ScenePropertyTypes.h>
#include <tge/script/BaseProperties.h>
#include <tge/script/Contexts/ScriptUpdateContext.h>
#include <tge/script/Script.h>
#include <tge/script/ScriptManager.h>
#include <tge/script/ScriptNodeTypeRegistry.h>
#include <tge/script/ScriptRuntimeInstance.h>

namespace
{
    std::string NormalizeAssetPath(std::string aPath)
    {
        std::replace(aPath.begin(), aPath.end(), '\\', '/');

        while (aPath.size() >= 2 && aPath[0] == '.' && aPath[1] == '/')
        {
            aPath.erase(aPath.begin(), aPath.begin() + 2);
        }

        while (!aPath.empty() && aPath.front() == '/')
        {
            aPath.erase(aPath.begin());
        }

        return aPath;
    }

    std::string NormalizeScriptId(std::string aPath)
    {
        aPath = NormalizeAssetPath(aPath);

        std::filesystem::path path = std::filesystem::path(aPath);
        if (path.has_extension() && path.extension() == ".tgscript")
        {
            return NormalizeAssetPath(path.replace_extension().string());
        }

        return aPath;
    }

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

    std::string GetClipStemFromPropertyName(std::string aPropertyName)
    {
        const std::string prefix = "clip_";
        if (aPropertyName.rfind(prefix, 0) == 0)
        {
            aPropertyName.erase(0, prefix.size());
        }

        return aPropertyName.empty() ? "idle" : aPropertyName;
    }

    bool TryConvertAnyToProperty(const std::string& aKey, const std::any& aValue, Tga::Property& outProperty)
    {
        if (const auto* asFloat = std::any_cast<float>(&aValue))
        {
            outProperty = Tga::Property::Create<float>(*asFloat);
            return true;
        }

        if (const auto* asInt = std::any_cast<int>(&aValue))
        {
            outProperty = Tga::Property::Create<int>(*asInt);
            return true;
        }

        if (const auto* asBool = std::any_cast<bool>(&aValue))
        {
            outProperty = Tga::Property::Create<bool>(*asBool);
            return true;
        }

        if (const auto* asVec3 = std::any_cast<Tga::Vector3f>(&aValue))
        {
            outProperty = Tga::Property::Create<Tga::Vector3f>(*asVec3);
            return true;
        }

        if (const auto* asCuVec3 = std::any_cast<CommonUtilities::Vector3<float>>(&aValue))
        {
            outProperty = Tga::Property::Create<Tga::Vector3f>(Tga::Vector3f(asCuVec3->x, asCuVec3->y, asCuVec3->z));
            return true;
        }

        if (const auto* asColor = std::any_cast<Tga::Color>(&aValue))
        {
            outProperty = Tga::Property::Create<Tga::Color>(*asColor);
            return true;
        }

        std::string textValue;
        if (TryReadStringFromAny(aValue, textValue))
        {
            textValue = NormalizeAssetPath(textValue);

            if (aKey.rfind("clip_", 0) == 0)
            {
                Tga::CopyOnWriteWrapper<Tga::AnimationClipReference> clipReference =
                    Tga::CopyOnWriteWrapper<Tga::AnimationClipReference>::Create();
                clipReference.Edit().path = Tga::StringRegistry::RegisterOrGetString(textValue.c_str());
                outProperty = Tga::Property::Create<Tga::CopyOnWriteWrapper<Tga::AnimationClipReference>>(clipReference);
                return true;
            }

            outProperty = Tga::Property::Create<Tga::StringId>(Tga::StringRegistry::RegisterOrGetString(textValue.c_str()));
            return true;
        }

        return false;
    }
}

bool AnimationGraphRuntime::Initialize(
    const std::string& aGraphPath,
    const std::string& aModelPropertyName,
    const std::unordered_map<std::string, std::any>& someSourceProperties,
    const std::string& aModelPathHint,
    AnimationEventQueue* anEventQueue)
{
    myGraphScriptId = NormalizeScriptId(aGraphPath);
    if (myGraphScriptId.empty())
    {
        return false;
    }

    myModelPathHint = NormalizeAssetPath(aModelPathHint);

    const std::string modelPropertyName = aModelPropertyName.empty() ? "model" : aModelPropertyName;
    myModelPropertyName = Tga::StringRegistry::RegisterOrGetString(modelPropertyName.c_str());

    std::string posePropertyName = myModelPropertyName.GetString();
    posePropertyName += "_pose";
    myPosePropertyName = Tga::StringRegistry::RegisterOrGetString(posePropertyName.c_str());

    myEventQueue = anEventQueue;

    myDynamicProperties.clear();
    myStaticProperties.clear();

    PopulateSourceProperties(someSourceProperties);
    EnsureCommonDefaults();

    myScript = Tga::ScriptManager::GetScript(myGraphScriptId);
    if (!myScript)
    {
        myScriptInstance.reset();
        return false;
    }

    myScriptInstance.emplace(myScript);
    myScriptInstance->Init();

    EnsureGraphReadPropertiesExist();

    myFrameNumber = 0;
    mySyncedTime = 0.0f;
    myPendingRootMotionTranslation = Tga::Vector3f();
    myPendingRootMotionRotation = Tga::Quatf();

    return true;
}

void AnimationGraphRuntime::Reset()
{
    if (myScriptInstance)
    {
        myScriptInstance->Reset();
    }

    myFrameNumber = 0;
    mySyncedTime = 0.0f;
    myPendingRootMotionTranslation = Tga::Vector3f();
    myPendingRootMotionRotation = Tga::Quatf();
}

bool AnimationGraphRuntime::IsReady() const
{
    return myScriptInstance.has_value() && static_cast<bool>(myScript);
}

bool AnimationGraphRuntime::Update(float aDeltaTime, const std::shared_ptr<Tga::Model>& aModel, Tga::LocalSpacePose& outPose)
{
    if (!myScriptInstance || !aModel)
    {
        return false;
    }

    Tga::ScriptUpdateContext updateContext;
    updateContext.deltaTime = aDeltaTime;
    updateContext.frameNumber = static_cast<int>(++myFrameNumber);
    updateContext.dynamicProperties = &myDynamicProperties;
    updateContext.staticProperties = &myStaticProperties;

    myScriptInstance->Update(updateContext);

    auto poseIt = myDynamicProperties.find(myPosePropertyName);
    if (poseIt == myDynamicProperties.end())
    {
        return false;
    }

    Tga::PoseAndMotion* poseAndMotion = poseIt->second.Get<Tga::PoseAndMotion>();
    if (!poseAndMotion || !poseAndMotion->poseGenerator)
    {
        myDynamicProperties.erase(poseIt);
        return false;
    }

    if (poseAndMotion->desiredSyncedPlaybackRateWeight > 0.0f)
    {
        mySyncedTime += poseAndMotion->desiredSyncedPlaybackRate * aDeltaTime;
        mySyncedTime -= std::floor(mySyncedTime);
        if (mySyncedTime < 0.0f)
        {
            mySyncedTime += 1.0f;
        }
    }

    Tga::PoseGenerationContext generationContext = {};
    generationContext.model = aModel;
    generationContext.frameNumber = myFrameNumber;
    generationContext.deltaTime = aDeltaTime;
    generationContext.syncedPlaybackTime = mySyncedTime;

    std::vector<Tga::EmittedAnimationEvent> emittedEvents;
    generationContext.emittedEvents = &emittedEvents;

    poseAndMotion->poseGenerator->GenerateRootMotionDelta(
        generationContext,
        myPendingRootMotionTranslation,
        myPendingRootMotionRotation);

    poseAndMotion->poseGenerator->GeneratePose(generationContext, outPose);

    if (myEventQueue)
    {
        for (const Tga::EmittedAnimationEvent& event : emittedEvents)
        {
            AnimationEventRecord record;
            record.id = event.id;
            record.clipPath = event.clipPath;
            record.time = event.time;
            myEventQueue->Push(record);
        }
    }

    myDynamicProperties.erase(poseIt);
    return true;
}

bool AnimationGraphRuntime::SetFloatParameter(const std::string& aName, const float aValue)
{
    return SetParameter<float>(aName, aValue);
}

bool AnimationGraphRuntime::SetIntParameter(const std::string& aName, const int aValue)
{
    return SetParameter<int>(aName, aValue);
}

bool AnimationGraphRuntime::SetBoolParameter(const std::string& aName, const bool aValue)
{
    return SetParameter<bool>(aName, aValue);
}

bool AnimationGraphRuntime::SetVector3Parameter(const std::string& aName, const Tga::Vector3f& aValue)
{
    return SetParameter<Tga::Vector3f>(aName, aValue);
}

bool AnimationGraphRuntime::SetStringParameter(const std::string& aName, const std::string& aValue)
{
    return SetParameter<Tga::StringId>(aName, Tga::StringRegistry::RegisterOrGetString(aValue.c_str()));
}

const std::unordered_map<Tga::StringId, Tga::Property>& AnimationGraphRuntime::GetParameters() const
{
    return myDynamicProperties;
}

Tga::Vector3f AnimationGraphRuntime::ConsumeRootMotionTranslation()
{
    const Tga::Vector3f delta = myPendingRootMotionTranslation;
    myPendingRootMotionTranslation = Tga::Vector3f();
    return delta;
}

Tga::Quatf AnimationGraphRuntime::ConsumeRootMotionRotation()
{
    const Tga::Quatf delta = myPendingRootMotionRotation;
    myPendingRootMotionRotation = Tga::Quatf();
    return delta;
}

void AnimationGraphRuntime::PopulateSourceProperties(const std::unordered_map<std::string, std::any>& someSourceProperties)
{
    for (const auto& [name, value] : someSourceProperties)
    {
        if (name.empty())
        {
            continue;
        }

        Tga::Property property;
        if (!TryConvertAnyToProperty(name, value, property) || !property.HasValue())
        {
            continue;
        }

        const Tga::StringId propertyName = Tga::StringRegistry::RegisterOrGetString(name.c_str());
        myDynamicProperties[propertyName] = property;

        if (name == "modelPath")
        {
            std::string path;
            if (TryReadStringFromAny(value, path))
            {
                myModelPathHint = NormalizeAssetPath(path);
            }
        }
        else if (name == "model")
        {
            std::string path;
            if (TryReadStringFromAny(value, path))
            {
                myModelPathHint = NormalizeAssetPath(path);
            }
        }
    }
}

void AnimationGraphRuntime::EnsureCommonDefaults()
{
    if (!myModelPathHint.empty())
    {
        myDynamicProperties[myModelPropertyName] = Tga::Property::Create<Tga::StringId>(
            Tga::StringRegistry::RegisterOrGetString(myModelPathHint.c_str()));

        const Tga::StringId modelPathName = Tga::StringRegistry::RegisterOrGetString("modelPath");
        myDynamicProperties[modelPathName] = Tga::Property::Create<Tga::StringId>(
            Tga::StringRegistry::RegisterOrGetString(myModelPathHint.c_str()));
    }

    if (myDynamicProperties.find(Tga::StringRegistry::RegisterOrGetString("anim_speed")) == myDynamicProperties.end())
    {
        myDynamicProperties[Tga::StringRegistry::RegisterOrGetString("anim_speed")] = Tga::Property::Create<float>(1.0f);
    }

    if (myDynamicProperties.find(Tga::StringRegistry::RegisterOrGetString("anim_direction")) == myDynamicProperties.end())
    {
        myDynamicProperties[Tga::StringRegistry::RegisterOrGetString("anim_direction")] = Tga::Property::Create<float>(0.0f);
    }

    if (myDynamicProperties.find(Tga::StringRegistry::RegisterOrGetString("anim_is_grounded")) == myDynamicProperties.end())
    {
        myDynamicProperties[Tga::StringRegistry::RegisterOrGetString("anim_is_grounded")] = Tga::Property::Create<bool>(true);
    }

    if (myDynamicProperties.find(Tga::StringRegistry::RegisterOrGetString("anim_state")) == myDynamicProperties.end())
    {
        myDynamicProperties[Tga::StringRegistry::RegisterOrGetString("anim_state")] = Tga::Property::Create<int>(0);
    }

    if (myDynamicProperties.find(Tga::StringRegistry::RegisterOrGetString("walk_jog_blend")) == myDynamicProperties.end())
    {
        myDynamicProperties[Tga::StringRegistry::RegisterOrGetString("walk_jog_blend")] = Tga::Property::Create<float>(0.0f);
    }
}

void AnimationGraphRuntime::EnsureGraphReadPropertiesExist()
{
    if (!myScript)
    {
        return;
    }

    for (Tga::ScriptNodeId nodeId = myScript->GetFirstNodeId();
         nodeId.id != Tga::ScriptNodeId::InvalidId;
         nodeId = myScript->GetNextNodeId(nodeId))
    {
        const std::string_view nodeType = Tga::ScriptNodeTypeRegistry::GetNodeTypeShortName(myScript->GetType(nodeId));

        Tga::StringId propertyName;
        if (!TryReadPropertyNamePin(nodeId, propertyName) || propertyName.IsEmpty())
        {
            continue;
        }

        if (nodeType == "Read Animation Clip Property")
        {
            const std::string propertyNameText = propertyName.GetString();
            std::string clipPath = BuildDefaultClipPathFromPropertyName(propertyNameText);

            auto clipPropertyIt = myDynamicProperties.find(propertyName);
            if (clipPropertyIt != myDynamicProperties.end())
            {
                if (const auto* clipRef = clipPropertyIt->second.Get<Tga::CopyOnWriteWrapper<Tga::AnimationClipReference>>())
                {
                    clipPath = clipRef->Get().path.GetString();
                }
            }

            clipPath = NormalizeAssetPath(clipPath);
            if (clipPath.empty())
            {
                clipPath = BuildDefaultClipPathFromPropertyName(propertyNameText);
            }

            Tga::CopyOnWriteWrapper<Tga::AnimationClipReference> clipReference =
                Tga::CopyOnWriteWrapper<Tga::AnimationClipReference>::Create();
            clipReference.Edit().path = Tga::StringRegistry::RegisterOrGetString(clipPath.c_str());

            myDynamicProperties[propertyName] =
                Tga::Property::Create<Tga::CopyOnWriteWrapper<Tga::AnimationClipReference>>(clipReference);

            continue;
        }

        if (myDynamicProperties.find(propertyName) != myDynamicProperties.end())
        {
            continue;
        }

        if (nodeType == "Read Float Property")
        {
            myDynamicProperties[propertyName] = Tga::Property::Create<float>(0.0f);
        }
        else if (nodeType == "Read Int Property")
        {
            myDynamicProperties[propertyName] = Tga::Property::Create<int>(0);
        }
        else if (nodeType == "Read Bool Property")
        {
            myDynamicProperties[propertyName] = Tga::Property::Create<bool>(false);
        }
        else if (nodeType == "Read String Property")
        {
            myDynamicProperties[propertyName] =
                Tga::Property::Create<Tga::StringId>(Tga::StringRegistry::RegisterOrGetString(""));
        }
        else if (nodeType == "Read Float3 Property")
        {
            myDynamicProperties[propertyName] = Tga::Property::Create<Tga::Vector3f>(Tga::Vector3f());
        }
    }
}

std::string AnimationGraphRuntime::BuildDefaultClipPathFromPropertyName(const std::string& aPropertyName) const
{
    const std::string clipStem = GetClipStemFromPropertyName(aPropertyName);
    return "AnimationClips/" + clipStem + ".tgac";
}

bool AnimationGraphRuntime::TryReadPropertyNamePin(Tga::ScriptNodeId aNodeId, Tga::StringId& outPropertyName) const
{
    if (!myScript)
    {
        return false;
    }

    std::size_t inputCount = 0;
    const Tga::ScriptPinId* inputPins = myScript->GetInputPins(aNodeId, inputCount);
    for (std::size_t i = 0; i < inputCount; ++i)
    {
        const Tga::ScriptPin& pin = myScript->GetPin(inputPins[i]);
        if (pin.name != "Name"_tgaid)
        {
            continue;
        }

        const Tga::Property& nameProperty = pin.overridenValue.HasValue() ? pin.overridenValue : pin.defaultValue;
        if (const Tga::StringId* name = nameProperty.Get<Tga::StringId>())
        {
            outPropertyName = *name;
            return true;
        }
    }

    return false;
}
