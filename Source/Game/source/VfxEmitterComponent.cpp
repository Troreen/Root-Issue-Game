//#include "VfxEmitterComponent.h"
//
//#include "GameObject.h"
//#include "PlayerController.h"
//#include "SchnozController.h"
//#include "VfxSystem.h"
//
//#include <algorithm>
//#include <cctype>
//#include <utility>
//
//namespace
//{
//    std::string ToLower(std::string value)
//    {
//        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
//        return value;
//    }
//
//    float ResolveOwnerForwardSign(const GameObject* anOwner)
//    {
//        if (!anOwner)
//        {
//            return 1.0f;
//        }
//
//        if (const auto* playerController = anOwner->GetComponent<PlayerController>())
//        {
//            return playerController->IsFacingRight() ? 1.0f : -1.0f;
//        }
//
//        if (const auto* schnozController = anOwner->GetComponent<SchnozController>())
//        {
//            return schnozController->IsFacingRight() ? 1.0f : -1.0f;
//        }
//
//        return 1.0f;
//    }
//}
//
//VfxEmitterComponent::VfxEmitterComponent(Settings someSettings)
//    : mySettings(std::move(someSettings))
//{
//    myTimeUntilNextSpawn = std::max(0.0f, mySettings.spawnIntervalSeconds);
//}
//
//void VfxEmitterComponent::Init(Tga::Engine& anEngine)
//{
//    (void)anEngine;
//
//    if (mySettings.effectId.empty())
//    {
//        SetEnabled(false);
//        return;
//    }
//
//    for (int index = 0; index < mySettings.burstOnStartCount; ++index)
//    {
//        EmitOnce();
//    }
//}
//
//void VfxEmitterComponent::Update(const float aDeltaTime)
//{
//    if (myEmitActivationBurstNextUpdate)
//    {
//        myEmitActivationBurstNextUpdate = false;
//        for (int index = 0; index < mySettings.burstOnActivateCount; ++index)
//        {
//            EmitOnce();
//        }
//    }
//
//    if (mySettings.spawnIntervalSeconds <= 0.0f)
//    {
//        return;
//    }
//
//    myTimeUntilNextSpawn -= std::max(0.0f, aDeltaTime);
//    while (myTimeUntilNextSpawn <= 0.0f)
//    {
//        EmitOnce();
//        myTimeUntilNextSpawn += mySettings.spawnIntervalSeconds;
//    }
//}
//
//void VfxEmitterComponent::OnActiveChanged(const bool isActive)
//{
//    if (!isActive || mySettings.burstOnActivateCount <= 0)
//    {
//        return;
//    }
//
//    // Defer to Update so owner transform changes made in the same frame are reflected.
//    myEmitActivationBurstNextUpdate = true;
//}
//
//void VfxEmitterComponent::EmitOnce()
//{
//    if (mySettings.effectId.empty())
//    {
//        return;
//    }
//
//    const std::string space = ToLower(mySettings.space);
//    if (space == "screen")
//    {
//        VfxService::SpawnScreenEffect(mySettings.effectId, mySettings.screenPosition, mySettings.sizeMultiplier);
//        return;
//    }
//
//    if (!GetOwner())
//    {
//        return;
//    }
//
//    VfxService::SpawnWorldEffect(
//        mySettings.effectId,
//        GetOwner()->GetTransform().GetPosition(),
//        mySettings.sizeMultiplier,
//        ResolveOwnerForwardSign(GetOwner()));
//}
