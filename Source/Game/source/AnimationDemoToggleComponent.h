#pragma once

#include "ScriptComponent.h"

#include <string>
#include <vector>

class AnimationGraphComponent;

class AnimationDemoToggleComponent final : public ScriptComponent
{
public:
    void SetClipOrder(const std::vector<std::string>& someClipPropertyNames);

protected:
    void OnStart() override;
    void OnUpdate(float aDeltaTime) override;

private:
    void ApplyActiveToggle();
    void LogActiveClip() const;
    bool UpdateBlendInput(float aDeltaTime);
    int GetNeighborClipIndex(bool aTowardsNext) const;
    static float MoveTowards(float aCurrent, float aTarget, float aMaxDelta);
    bool WasJustPressed(int aVirtualKey, bool& wasDown) const;
    void BuildDefaultClipOrder();

    std::vector<std::string> myClipPropertyNames;
    std::vector<std::string> myWeightPropertyNames;

    AnimationGraphComponent* myGraph = nullptr;
    int myActiveIndex = 0;
    float myBlendToNeighbor = 0.0f;

    bool myPrevNextDown = false;
    bool myPrevPrevDown = false;
    bool myPrevIdleDown = false;
};
