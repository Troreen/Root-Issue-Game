#pragma once

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include <tge/animation/Pose.h>
#include <tge/math/Quaternion.h>
#include <tge/math/Vector3.h>
#include <tge/script/Property.h>
#include <tge/script/ScriptCommon.h>
#include <tge/script/ScriptRuntimeInstance.h>
#include <tge/stringRegistry/StringRegistry.h>

class AnimationEventQueue;

namespace Tga
{
    class Model;
    class Script;
}

class AnimationGraphRuntime
{
public:
    bool Initialize(
        const std::string& aGraphPath,
        const std::string& aModelPropertyName,
        const std::unordered_map<std::string, std::any>& someSourceProperties,
        const std::string& aModelPathHint,
        AnimationEventQueue* anEventQueue);

    void Reset();

    bool IsReady() const;

    bool Update(float aDeltaTime, const std::shared_ptr<Tga::Model>& aModel, Tga::LocalSpacePose& outPose);

    bool SetFloatParameter(const std::string& aName, float aValue);
    bool SetIntParameter(const std::string& aName, int aValue);
    bool SetBoolParameter(const std::string& aName, bool aValue);
    bool SetVector3Parameter(const std::string& aName, const Tga::Vector3f& aValue);
    bool SetStringParameter(const std::string& aName, const std::string& aValue);

    const std::unordered_map<Tga::StringId, Tga::Property>& GetParameters() const;

    Tga::Vector3f ConsumeRootMotionTranslation();
    Tga::Quatf ConsumeRootMotionRotation();

private:
    void PopulateSourceProperties(const std::unordered_map<std::string, std::any>& someSourceProperties);
    void EnsureCommonDefaults();
    void EnsureGraphReadPropertiesExist();

    std::string BuildDefaultClipPathFromPropertyName(const std::string& aPropertyName) const;

    bool TryReadPropertyNamePin(Tga::ScriptNodeId aNodeId, Tga::StringId& outPropertyName) const;

    template <typename T>
    bool SetParameter(const std::string& aName, const T& aValue)
    {
        if (aName.empty())
        {
            return false;
        }

        const Tga::StringId propertyName = Tga::StringRegistry::RegisterOrGetString(aName.c_str());
        myDynamicProperties[propertyName] = Tga::Property::Create<T>(aValue);
        return true;
    }

    std::string myGraphScriptId;
    std::string myModelPathHint;

    Tga::StringId myModelPropertyName;
    Tga::StringId myPosePropertyName;

    std::shared_ptr<const Tga::Script> myScript;
    std::optional<Tga::ScriptRuntimeInstance> myScriptInstance;

    std::unordered_map<Tga::StringId, Tga::Property> myDynamicProperties;
    std::unordered_map<Tga::StringId, Tga::Property> myStaticProperties;

    AnimationEventQueue* myEventQueue = nullptr;

    unsigned int myFrameNumber = 0;
    float mySyncedTime = 0.0f;

    Tga::Vector3f myPendingRootMotionTranslation = {};
    Tga::Quatf myPendingRootMotionRotation = {};
};
