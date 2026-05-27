#include "AnimationDemoToggleComponent.h"

#include "AnimationGraphComponent.h"
#include "GameObject.h"

#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
    constexpr float BlendInputSpeed = 2.0f;
    constexpr float BlendReturnSpeed = 4.0f;

    std::string WeightNameFromClipProperty(const std::string& aClipPropertyName)
    {
        constexpr const char* clipPrefix = "clip_";
        if (aClipPropertyName.rfind(clipPrefix, 0) == 0)
        {
            return "w_" + aClipPropertyName.substr(5);
        }

        return "w_" + aClipPropertyName;
    }
}

void AnimationDemoToggleComponent::SetClipOrder(const std::vector<std::string>& someClipPropertyNames)
{
    myClipPropertyNames = someClipPropertyNames;
    myWeightPropertyNames.clear();
    myBlendToNeighbor = 0.0f;

    if (myClipPropertyNames.empty())
    {
        return;
    }

    for (std::size_t i = 1; i < myClipPropertyNames.size(); ++i)
    {
        myWeightPropertyNames.push_back(WeightNameFromClipProperty(myClipPropertyNames[i]));
    }

    myActiveIndex = std::clamp(myActiveIndex, 0, static_cast<int>(myClipPropertyNames.size() - 1));
}

void AnimationDemoToggleComponent::BuildDefaultClipOrder()
{
    SetClipOrder(
        {
            "clip_idle",
            "clip_walk",
            "clip_run",
            "clip_attack_basic01",
        });
}

bool AnimationDemoToggleComponent::WasJustPressed(const int aVirtualKey, bool& wasDown) const
{
    const bool isDown = (GetAsyncKeyState(aVirtualKey) & 0x8000) != 0;
    const bool justPressed = isDown && !wasDown;
    wasDown = isDown;
    return justPressed;
}

void AnimationDemoToggleComponent::OnStart()
{
    if (myClipPropertyNames.empty())
    {
        BuildDefaultClipOrder();
    }

    if (GameObject* owner = GetOwner())
    {
        myGraph = owner->GetComponent<AnimationGraphComponent>();
    }

    ApplyActiveToggle();
    LogActiveClip();
}

void AnimationDemoToggleComponent::OnUpdate(const float aDeltaTime)
{
    if (!myGraph || myClipPropertyNames.empty())
    {
        return;
    }

    bool changed = false;
    bool activeClipChanged = false;

    /*if (WasJustPressed(VK_RIGHT, myPrevNextDown))
    {
        myActiveIndex = (myActiveIndex + 1) % static_cast<int>(myClipPropertyNames.size());
        myBlendToNeighbor = 0.0f;
        changed = true;
        activeClipChanged = true;
    }

    if (WasJustPressed(VK_LEFT, myPrevPrevDown))
    {
        --myActiveIndex;
        if (myActiveIndex < 0)
        {
            myActiveIndex = static_cast<int>(myClipPropertyNames.size() - 1);
        }
        myBlendToNeighbor = 0.0f;
        changed = true;
        activeClipChanged = true;
    }

    if (WasJustPressed(VK_HOME, myPrevIdleDown))
    {
        myActiveIndex = 0;
        myBlendToNeighbor = 0.0f;
        changed = true;
        activeClipChanged = true;
    }*/

    if (UpdateBlendInput(aDeltaTime))
    {
        changed = true;
    }

    if (changed)
    {
        ApplyActiveToggle();

        if (activeClipChanged)
        {
            LogActiveClip();
        }
    }
}

void AnimationDemoToggleComponent::ApplyActiveToggle()
{
    if (!myGraph)
    {
        return;
    }

    std::vector<float> blendedWeights(myWeightPropertyNames.size(), 0.0f);

    auto addClipInfluence = [&blendedWeights](const int aClipIndex, const float anInfluence)
    {
        if (anInfluence <= 0.0f || aClipIndex <= 0)
        {
            return;
        }

        const std::size_t weightIndex = static_cast<std::size_t>(aClipIndex - 1);
        if (weightIndex < blendedWeights.size())
        {
            blendedWeights[weightIndex] += anInfluence;
        }
    };

    int neighborIndex = myActiveIndex;
    float neighborBlend = 0.0f;

    if (myBlendToNeighbor > 0.0f)
    {
        neighborIndex = GetNeighborClipIndex(true);
        if (neighborIndex != myActiveIndex)
        {
            neighborBlend = myBlendToNeighbor;
        }
    }
    else if (myBlendToNeighbor < 0.0f)
    {
        neighborIndex = GetNeighborClipIndex(false);
        if (neighborIndex != myActiveIndex)
        {
            neighborBlend = -myBlendToNeighbor;
        }
    }

    neighborBlend = std::clamp(neighborBlend, 0.0f, 1.0f);
    const float currentBlend = 1.0f - neighborBlend;

    addClipInfluence(myActiveIndex, currentBlend);
    addClipInfluence(neighborIndex, neighborBlend);

    for (std::size_t i = 0; i < myWeightPropertyNames.size(); ++i)
    {
        myGraph->SetFloatParameter(myWeightPropertyNames[i], blendedWeights[i]);
    }

    myGraph->SetFloatParameter("anim_speed", 1.0f);
}

void AnimationDemoToggleComponent::LogActiveClip() const
{
    if (myClipPropertyNames.empty())
    {
        return;
    }

    const std::string& clipName = myClipPropertyNames[static_cast<std::size_t>(myActiveIndex)];
    std::cout
        << "[AnimationDemo] Active clip property: " << clipName
        << "  (LEFT/RIGHT to switch, HOME to idle, HOLD UP/DOWN to blend next/prev)\n";
}

bool AnimationDemoToggleComponent::UpdateBlendInput(const float aDeltaTime)
{
    const bool upHeld = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
    const bool downHeld = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;

    float targetBlend = 0.0f;
    if (upHeld != downHeld)
    {
        targetBlend = upHeld ? 1.0f : -1.0f;
    }

    if (targetBlend > 0.0f && GetNeighborClipIndex(true) == myActiveIndex)
    {
        targetBlend = 0.0f;
    }
    else if (targetBlend < 0.0f && GetNeighborClipIndex(false) == myActiveIndex)
    {
        targetBlend = 0.0f;
    }

    const float speed = (targetBlend == 0.0f) ? BlendReturnSpeed : BlendInputSpeed;
    const float nextBlend = MoveTowards(myBlendToNeighbor, targetBlend, speed * aDeltaTime);

    if (std::abs(nextBlend - myBlendToNeighbor) < 0.0001f)
    {
        return false;
    }

    myBlendToNeighbor = nextBlend;
    return true;
}

int AnimationDemoToggleComponent::GetNeighborClipIndex(const bool aTowardsNext) const
{
    if (myClipPropertyNames.empty())
    {
        return 0;
    }

    if (aTowardsNext)
    {
        const int candidate = myActiveIndex + 1;
        return (candidate < static_cast<int>(myClipPropertyNames.size())) ? candidate : myActiveIndex;
    }

    const int candidate = myActiveIndex - 1;
    return (candidate >= 0) ? candidate : myActiveIndex;
}

float AnimationDemoToggleComponent::MoveTowards(const float aCurrent, const float aTarget, const float aMaxDelta)
{
    if (aMaxDelta <= 0.0f)
    {
        return aCurrent;
    }

    const float delta = aTarget - aCurrent;
    if (std::abs(delta) <= aMaxDelta)
    {
        return aTarget;
    }

    return aCurrent + ((delta > 0.0f) ? aMaxDelta : -aMaxDelta);
}
