#pragma once

#include "Component.h"

#include <CommonUtilities/Vector2.hpp>
#include <CommonUtilities/Vector3.hpp>
#include <tge/math/Color.h>
#include <tge/sprite/sprite.h>

#include <string>
#include <unordered_map>

namespace Tga
{
    class Engine;
    class Texture;
}

/// Renders a reusable world-space animated sprite from a JSON sprite-sheet definition.
class AnimatedSpriteComponent final : public Component
{
public:
    explicit AnimatedSpriteComponent(std::string aDefinitionId);

    void Init(Tga::Engine& anEngine) override;
    void Update(float aDeltaTime) override;
    void Render() override;

    void SetDefinitionId(const std::string& aDefinitionId);
    const std::string& GetDefinitionId() const;

private:
    struct AnimatedSpriteDefinition
    {
        std::string id;
        std::string texturePath;
        int columns = 1;
        int rows = 1;
        int frameCount = 1;
        float fps = 24.0f;
        bool loop = true;
        CommonUtilities::Vector2<float> size = { 120.0f, 120.0f };
        CommonUtilities::Vector3<float> worldOffset = { 0.0f, 0.0f, 0.0f };
        Tga::Color color = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    struct FrameSample
    {
        Tga::TextureRext textureRect = { 0.0f, 0.0f, 1.0f, 1.0f };
    };

    static bool EnsureDefinitionsLoaded();
    static bool LoadDefinitions();
    static bool LoadDefinitionFromFile(const std::string& aPath);
    static std::string NormalizeTexturePath(const std::string& aTexturePath);

    static FrameSample ComputeFrameSample(
        int aFrameCount,
        float aFps,
        bool aLoop,
        int aColumns,
        int aRows,
        float anAgeSeconds);

    void RefreshRuntimeResources();
    void DisableRuntime();

    std::string myDefinitionId;
    const AnimatedSpriteDefinition* myDefinition = nullptr;
    const Tga::Texture* myTexture = nullptr;

    float myAgeSeconds = 0.0f;
    int myCachedFrameIndex = -1;

    CommonUtilities::Vector3<float> myCachedWorldPosition = { 0.0f, 0.0f, 0.0f };
    float myCachedWorldScale = -1.0f;
    float myCachedWorldRadius = 1.0f;
    bool myHasCachedTransform = false;

    Tga::SpriteSharedData mySharedData;
    Tga::Sprite3DInstanceData myInstanceData;

    bool myCanRender = false;
    bool myWarnedMissingDefinition = false;
    bool myWarnedMissingTexture = false;

    static std::unordered_map<std::string, AnimatedSpriteDefinition> ourDefinitions;
    static bool ourDefinitionsLoaded;
};