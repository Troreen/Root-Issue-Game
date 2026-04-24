//#include "AnimatedSpriteComponent.h"
//
//#include "GameObject.h"
//
//#include <tge/drawers/SpriteDrawer.h>
//#include <tge/engine.h>
//#include <tge/graphics/Camera.h>
//#include <tge/graphics/GraphicsEngine.h>
//#include <tge/graphics/GraphicsStateStack.h>
//#include <tge/settings/Settings.h>
//#include <tge/texture/TextureManager.h>
//
//#include <CommonUtilities/Plane.hpp>
//
//#include <nlohmann/json.hpp>
//
//#include <algorithm>
//#include <array>
//#include <cstdint>
//#include <cmath>
//#include <filesystem>
//#include <fstream>
//#include <iostream>
//#include <unordered_map>
//
//namespace
//{
//    using Json = nlohmann::json;
//    constexpr const char* kAnimatedSpriteDefinitionFolder = "Animations/Sprites";
//    constexpr const char* kDefaultTexturePrefix = "Animations/";
//    constexpr float kCullingRadiusPadding = 10.0f;
//    constexpr bool kEnableAnimatedSpriteFrustumCulling = true;
//
//    struct ViewFrustum
//    {
//        Tga::Matrix4x4f viewMatrix;
//        std::array<CommonUtilities::Plane<float>, 6> planes;
//    };
//
//    struct AnimatedSpritePerfCounters
//    {
//        float lastLogTime = -1.0f;
//        uint32_t submittedAccum = 0;
//        uint32_t culledAccum = 0;
//    };
//
//    AnimatedSpritePerfCounters gAnimatedSpritePerfCounters;
//
//    float ClampPositive(const float aValue, const float aFallback)
//    {
//        return aValue > 0.0f ? aValue : aFallback;
//    }
//
//    int ClampPositive(const int aValue, const int aFallback)
//    {
//        return aValue > 0 ? aValue : aFallback;
//    }
//
//    template <typename T>
//    T MaxValue(const T aLeft, const T aRight)
//    {
//        return aLeft > aRight ? aLeft : aRight;
//    }
//
//    CommonUtilities::Vector2<float> ParseVector2(const Json& aJsonValue, const CommonUtilities::Vector2<float>& aFallback)
//    {
//        if (!aJsonValue.is_array() || aJsonValue.size() < 2)
//        {
//            return aFallback;
//        }
//
//        if (!aJsonValue[0].is_number() || !aJsonValue[1].is_number())
//        {
//            return aFallback;
//        }
//
//        return {
//            aJsonValue[0].get<float>(),
//            aJsonValue[1].get<float>()
//        };
//    }
//
//    CommonUtilities::Vector3<float> ParseVector3(const Json& aJsonValue, const CommonUtilities::Vector3<float>& aFallback)
//    {
//        if (!aJsonValue.is_array() || aJsonValue.size() < 3)
//        {
//            return aFallback;
//        }
//
//        if (!aJsonValue[0].is_number() || !aJsonValue[1].is_number() || !aJsonValue[2].is_number())
//        {
//            return aFallback;
//        }
//
//        return {
//            aJsonValue[0].get<float>(),
//            aJsonValue[1].get<float>(),
//            aJsonValue[2].get<float>()
//        };
//    }
//
//    Tga::Color ParseColor(const Json& aJsonValue, const Tga::Color& aFallback)
//    {
//        if (!aJsonValue.is_array() || aJsonValue.size() < 4)
//        {
//            return aFallback;
//        }
//
//        if (!aJsonValue[0].is_number() || !aJsonValue[1].is_number() || !aJsonValue[2].is_number() || !aJsonValue[3].is_number())
//        {
//            return aFallback;
//        }
//
//        return {
//            aJsonValue[0].get<float>(),
//            aJsonValue[1].get<float>(),
//            aJsonValue[2].get<float>(),
//            aJsonValue[3].get<float>()
//        };
//    }
//
//    CommonUtilities::Vector3<float> ToCommonVector3(const Tga::Vector3f& aVector)
//    {
//        return { aVector.x, aVector.y, aVector.z };
//    }
//
//    ViewFrustum BuildViewFrustum(const Tga::Camera& aCamera)
//    {
//        ViewFrustum frustum;
//        frustum.viewMatrix = aCamera.GetTransform().GetInverse();
//
//        const Tga::Matrix4x4f& projection = aCamera.GetProjection();
//        const float xScale = projection(1, 1);
//        const float yScale = projection(2, 2);
//        float nearPlane = 0.1f;
//        float farPlane = 50000.0f;
//        aCamera.GetProjectionPlanes(nearPlane, farPlane);
//
//        frustum.planes[0].InitWithPointAndNormal({ 0.0f, 0.0f, 0.0f }, { -xScale, 0.0f, -1.0f });
//        frustum.planes[1].InitWithPointAndNormal({ 0.0f, 0.0f, 0.0f }, { xScale, 0.0f, -1.0f });
//        frustum.planes[2].InitWithPointAndNormal({ 0.0f, 0.0f, 0.0f }, { 0.0f, -yScale, -1.0f });
//        frustum.planes[3].InitWithPointAndNormal({ 0.0f, 0.0f, 0.0f }, { 0.0f, yScale, -1.0f });
//        frustum.planes[4].InitWithPointAndNormal({ 0.0f, 0.0f, nearPlane }, { 0.0f, 0.0f, -1.0f });
//        frustum.planes[5].InitWithPointAndNormal({ 0.0f, 0.0f, farPlane }, { 0.0f, 0.0f, 1.0f });
//
//        return frustum;
//    }
//
//    bool IsSphereVisibleInViewFrustum(const Tga::Vector3f& aWorldCenter, const float aWorldRadius, const ViewFrustum& aFrustum)
//    {
//        if (aWorldRadius <= 0.0f)
//        {
//            return true;
//        }
//
//        const Tga::Vector3f viewCenter = aWorldCenter * aFrustum.viewMatrix;
//        const CommonUtilities::Vector3<float> viewCenterCommon = ToCommonVector3(viewCenter);
//
//        for (const CommonUtilities::Plane<float>& plane : aFrustum.planes)
//        {
//            if (plane.GetDistanceToPoint(viewCenterCommon) <= aWorldRadius)
//            {
//                continue;
//            }
//
//            return false;
//        }
//
//        return true;
//    }
//
//    bool TryGetCachedViewFrustum(const Tga::Engine& anEngine, ViewFrustum& anOutFrustum)
//    {
//        static float sCachedFrameTime = -1.0f;
//        static ViewFrustum sCachedFrustum;
//        static bool sHasFrustum = false;
//
//        const float frameTime = anEngine.GetTotalTime();
//        if (!sHasFrustum || frameTime != sCachedFrameTime)
//        {
//            const Tga::Camera& activeCamera = anEngine.GetGraphicsEngine().GetGraphicsStateStack().GetCamera();
//            sCachedFrustum = BuildViewFrustum(activeCamera);
//            sCachedFrameTime = frameTime;
//            sHasFrustum = true;
//        }
//
//        anOutFrustum = sCachedFrustum;
//        return true;
//    }
//
//    bool IsVisibleInCurrentCamera(const Tga::Engine& anEngine, const CommonUtilities::Vector3<float>& aWorldCenter, const float aWorldRadius)
//    {
//        ViewFrustum frustum;
//        if (!TryGetCachedViewFrustum(anEngine, frustum))
//        {
//            return true;
//        }
//
//        return IsSphereVisibleInViewFrustum({ aWorldCenter.x, aWorldCenter.y, aWorldCenter.z }, aWorldRadius, frustum);
//    }
//
//    int ComputeFrameIndex(const int aFrameCount, const float aFps, const bool aLoop, const float anAgeSeconds)
//    {
//        const int frameCount = ClampPositive(aFrameCount, 1);
//        int frameIndex = static_cast<int>(std::floor(MaxValue(0.0f, anAgeSeconds) * ClampPositive(aFps, 1.0f)));
//        if (aLoop)
//        {
//            frameIndex %= frameCount;
//        }
//        else
//        {
//            frameIndex = std::clamp(frameIndex, 0, frameCount - 1);
//        }
//        return frameIndex;
//    }
//}
//
//std::unordered_map<std::string, AnimatedSpriteComponent::AnimatedSpriteDefinition> AnimatedSpriteComponent::ourDefinitions;
//bool AnimatedSpriteComponent::ourDefinitionsLoaded = false;
//
//AnimatedSpriteComponent::AnimatedSpriteComponent(std::string aDefinitionId)
//    : myDefinitionId(std::move(aDefinitionId))
//{
//}
//
//void AnimatedSpriteComponent::Init(Tga::Engine& /*anEngine*/)
//{
//    RefreshRuntimeResources();
//}
//
//void AnimatedSpriteComponent::Update(const float aDeltaTime)
//{
//    if (!myCanRender || !myDefinition)
//    {
//        return;
//    }
//
//    GameObject* owner = GetOwner();
//    if (!owner)
//    {
//        return;
//    }
//
//    const auto& ownerTransform = owner->GetTransform();
//    const CommonUtilities::Vector3<float> ownerPosition = ownerTransform.GetPosition();
//    const CommonUtilities::Vector3<float> ownerScale = ownerTransform.GetScale();
//
//    const float absScaleX = std::abs(ownerScale.x);
//    const float absScaleY = std::abs(ownerScale.y);
//    const float worldScale = MaxValue(0.01f, (absScaleX + absScaleY) * 0.5f);
//    const CommonUtilities::Vector3<float> worldPosition = {
//        ownerPosition.x + myDefinition->worldOffset.x,
//        ownerPosition.y + myDefinition->worldOffset.y,
//        ownerPosition.z + myDefinition->worldOffset.z
//    };
//
//    const bool transformChanged =
//        !myHasCachedTransform ||
//        worldPosition.x != myCachedWorldPosition.x ||
//        worldPosition.y != myCachedWorldPosition.y ||
//        worldPosition.z != myCachedWorldPosition.z ||
//        worldScale != myCachedWorldScale;
//
//    myAgeSeconds += MaxValue(0.0f, aDeltaTime);
//
//    const int frameIndex = ComputeFrameIndex(
//        myDefinition->frameCount,
//        myDefinition->fps,
//        myDefinition->loop,
//        myAgeSeconds);
//
//    if (frameIndex != myCachedFrameIndex)
//    {
//        const FrameSample frame = ComputeFrameSample(
//            myDefinition->frameCount,
//            myDefinition->fps,
//            myDefinition->loop,
//            myDefinition->columns,
//            myDefinition->rows,
//            myAgeSeconds);
//
//        myInstanceData.myTextureRect = frame.textureRect;
//        myCachedFrameIndex = frameIndex;
//    }
//
//    if (transformChanged)
//    {
//        const float baseSize = (myDefinition->size.x + myDefinition->size.y) * 0.5f;
//        const float finalSize = MaxValue(0.01f, baseSize * worldScale);
//
//        myInstanceData.myTransform = Tga::Matrix4x4f::CreateFromScale(finalSize);
//        myInstanceData.myTransform.SetPosition({
//            worldPosition.x,
//            worldPosition.y,
//            worldPosition.z
//        });
//
//        myCachedWorldPosition = worldPosition;
//        myCachedWorldScale = worldScale;
//        myCachedWorldRadius = MaxValue(0.01f, finalSize * 0.5f + kCullingRadiusPadding);
//        myHasCachedTransform = true;
//    }
//
//    myInstanceData.myColor = myDefinition->color;
//}
//
//void AnimatedSpriteComponent::Render()
//{
//    if (!myCanRender || !myTexture)
//    {
//        return;
//    }
//
//    Tga::Engine* engine = Tga::Engine::GetInstance();
//    if (!engine)
//    {
//        return;
//    }
//
//    if constexpr (kEnableAnimatedSpriteFrustumCulling)
//    {
//        if (myHasCachedTransform && !IsVisibleInCurrentCamera(*engine, myCachedWorldPosition, myCachedWorldRadius))
//        {
//            ++gAnimatedSpritePerfCounters.culledAccum;
//            return;
//        }
//    }
//
//    mySharedData.myTexture = myTexture;
//    engine->GetGraphicsEngine().GetSpriteDrawer().EnqueueWorldSprite(mySharedData, myInstanceData);
//    ++gAnimatedSpritePerfCounters.submittedAccum;
//}
//
//void AnimatedSpriteComponent::SetDefinitionId(const std::string& aDefinitionId)
//{
//    if (myDefinitionId == aDefinitionId)
//    {
//        return;
//    }
//
//    myDefinitionId = aDefinitionId;
//    RefreshRuntimeResources();
//}
//
//const std::string& AnimatedSpriteComponent::GetDefinitionId() const
//{
//    return myDefinitionId;
//}
//
//bool AnimatedSpriteComponent::EnsureDefinitionsLoaded()
//{
//    if (ourDefinitionsLoaded)
//    {
//        return true;
//    }
//
//    ourDefinitionsLoaded = LoadDefinitions();
//    return ourDefinitionsLoaded;
//}
//
//bool AnimatedSpriteComponent::LoadDefinitions()
//{
//    namespace fs = std::filesystem;
//
//    ourDefinitions.clear();
//
//    const fs::path rootPath = Tga::Settings::GameAssetRoot() / kAnimatedSpriteDefinitionFolder;
//    if (!fs::exists(rootPath))
//    {
//        std::cout << "[AnimatedSprite] Definition folder missing: " << rootPath.string() << "\n";
//        return true;
//    }
//
//    int loadedCount = 0;
//    for (const auto& entry : fs::recursive_directory_iterator(rootPath))
//    {
//        if (!entry.is_regular_file())
//        {
//            continue;
//        }
//
//        const fs::path extension = entry.path().extension();
//        if (extension != ".tgvfx" && extension != ".json")
//        {
//            continue;
//        }
//
//        if (LoadDefinitionFromFile(entry.path().string()))
//        {
//            ++loadedCount;
//        }
//    }
//
//    std::cout << "[AnimatedSprite] Loaded " << loadedCount << " animated sprite definitions\n";
//    return true;
//}
//
//bool AnimatedSpriteComponent::LoadDefinitionFromFile(const std::string& aPath)
//{
//    std::ifstream input(aPath);
//    if (!input.is_open())
//    {
//        return false;
//    }
//
//    Json json;
//    try
//    {
//        input >> json;
//    }
//    catch (const std::exception&)
//    {
//        std::cout << "[AnimatedSprite] Failed to parse " << aPath << "\n";
//        return false;
//    }
//
//    AnimatedSpriteDefinition definition;
//
//    definition.id = json.value("id", std::filesystem::path(aPath).stem().string());
//    definition.texturePath = NormalizeTexturePath(json.value("texture", std::string{}));
//    definition.columns = ClampPositive(json.value("columns", 1), 1);
//    definition.rows = ClampPositive(json.value("rows", 1), 1);
//    definition.frameCount = ClampPositive(json.value("frameCount", definition.columns * definition.rows), definition.columns * definition.rows);
//    definition.fps = ClampPositive(json.value("fps", 24.0f), 24.0f);
//    definition.loop = json.value("loop", true);
//    definition.size = ParseVector2(json.value("size", Json::array()), { 120.0f, 120.0f });
//    definition.worldOffset = ParseVector3(
//        json.contains("worldOffset") ? json["worldOffset"] : json.value("spawnOffsetWorld", Json::array()),
//        { 0.0f, 0.0f, 0.0f });
//    definition.color = ParseColor(json.value("color", Json::array()), { 1.0f, 1.0f, 1.0f, 1.0f });
//
//    if (definition.id.empty() || definition.texturePath.empty())
//    {
//        std::cout << "[AnimatedSprite] Invalid definition: " << aPath << "\n";
//        return false;
//    }
//
//    ourDefinitions[definition.id] = std::move(definition);
//    return true;
//}
//
//std::string AnimatedSpriteComponent::NormalizeTexturePath(const std::string& aTexturePath)
//{
//    if (aTexturePath.empty())
//    {
//        return {};
//    }
//
//    if (aTexturePath.find('/') == std::string::npos &&
//        aTexturePath.find('\\') == std::string::npos)
//    {
//        return std::string(kDefaultTexturePrefix) + aTexturePath;
//    }
//
//    return aTexturePath;
//}
//
//AnimatedSpriteComponent::FrameSample AnimatedSpriteComponent::ComputeFrameSample(
//    const int aFrameCount,
//    const float aFps,
//    const bool aLoop,
//    const int aColumns,
//    const int aRows,
//    const float anAgeSeconds)
//{
//    const int columns = ClampPositive(aColumns, 1);
//    const int rows = ClampPositive(aRows, 1);
//    const int frameCount = ClampPositive(aFrameCount, 1);
//    const int sheetCapacity = MaxValue(1, columns * rows);
//    const int effectiveFrameCount = (std::min)(frameCount, sheetCapacity);
//
//    int frameIndex = static_cast<int>(std::floor(MaxValue(0.0f, anAgeSeconds) * ClampPositive(aFps, 1.0f)));
//    if (aLoop)
//    {
//        frameIndex %= effectiveFrameCount;
//    }
//    else
//    {
//        frameIndex = std::clamp(frameIndex, 0, effectiveFrameCount - 1);
//    }
//
//    const int localFrame = std::clamp(frameIndex, 0, sheetCapacity - 1);
//    const int col = localFrame % columns;
//    const int row = std::clamp(localFrame / columns, 0, rows - 1);
//
//    const float u0 = static_cast<float>(col) / static_cast<float>(columns);
//    const float v0 = static_cast<float>(row) / static_cast<float>(rows);
//    const float u1 = static_cast<float>(col + 1) / static_cast<float>(columns);
//    const float v1 = static_cast<float>(row + 1) / static_cast<float>(rows);
//
//    return {
//        { u0, v0, u1, v1 }
//    };
//}
//
//void AnimatedSpriteComponent::RefreshRuntimeResources()
//{
//    DisableRuntime();
//    myAgeSeconds = 0.0f;
//
//    if (myDefinitionId.empty())
//    {
//        return;
//    }
//
//    EnsureDefinitionsLoaded();
//
//    const auto definitionIt = ourDefinitions.find(myDefinitionId);
//    if (definitionIt == ourDefinitions.end())
//    {
//        if (!myWarnedMissingDefinition)
//        {
//            std::cout << "[AnimatedSprite] Unknown definition id: " << myDefinitionId << "\n";
//            myWarnedMissingDefinition = true;
//        }
//        return;
//    }
//
//    myDefinition = &definitionIt->second;
//
//    Tga::Engine* engine = Tga::Engine::GetInstance();
//    if (!engine)
//    {
//        return;
//    }
//
//    myTexture = engine->GetTextureManager().GetTexture(myDefinition->texturePath.c_str());
//    if (!myTexture)
//    {
//        if (!myWarnedMissingTexture)
//        {
//            std::cout << "[AnimatedSprite] Missing texture: " << myDefinition->texturePath << "\n";
//            myWarnedMissingTexture = true;
//        }
//        myDefinition = nullptr;
//        return;
//    }
//
//    myWarnedMissingDefinition = false;
//    myWarnedMissingTexture = false;
//    myCanRender = true;
//}
//
//void AnimatedSpriteComponent::DisableRuntime()
//{
//    myCanRender = false;
//    myDefinition = nullptr;
//    myTexture = nullptr;
//    myCachedFrameIndex = -1;
//    myCachedWorldPosition = { 0.0f, 0.0f, 0.0f };
//    myCachedWorldScale = -1.0f;
//    myCachedWorldRadius = 1.0f;
//    myHasCachedTransform = false;
//    mySharedData = {};
//    myInstanceData = {};
//}
