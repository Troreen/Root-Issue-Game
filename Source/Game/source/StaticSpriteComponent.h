#pragma once

#include "Component.h"
#include "SceneObjectData.h"

#include <tge/render/RenderCommon.h>
#include <tge/sprite/sprite.h>

#include <array>
#include <memory>
#include <string>

namespace Tga
{
    class SpriteShader;
    class TextureResource;
}

class StaticSpriteComponent final : public Component
{
public:
    explicit StaticSpriteComponent(SceneSpriteData someSpriteData);
    ~StaticSpriteComponent() override;

    void Init(Tga::Engine& anEngine) override;
    void Render() override;

    void SetTranslationRotationOffset(const Tga::Vector3f& aTranslation, const Tga::Vector3f& aRotations);

private:
    SceneSpriteData mySpriteData;
    const Tga::TextureResource* myTexture = nullptr;
    std::array<const Tga::TextureResource*, Tga::MAP_MAX> myMapTextures = {};
    Tga::SpriteSharedData mySharedData;
    Tga::Sprite3DInstanceData myInstanceData;
    std::unique_ptr<Tga::SpriteShader> myPbrSpriteShader;
    Tga::Matrix4x4f myRotationOffset;
    bool myWarnedMissingTexture = false;
    bool myHasMaterialMaps = false;
    bool myPbrSpriteShaderInitialized = false;
};
