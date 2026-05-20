#pragma once

#include "Component.h"
#include "SceneObjectData.h"

#include <tge/sprite/sprite.h>

#include <string>

namespace Tga
{
    class TextureResource;
}

class StaticSpriteComponent final : public Component
{
public:
    explicit StaticSpriteComponent(SceneSpriteData someSpriteData);

    void Init(Tga::Engine& anEngine) override;
    void Render() override;

private:
    SceneSpriteData mySpriteData;
    const Tga::TextureResource* myTexture = nullptr;
    Tga::SpriteSharedData mySharedData;
    Tga::Sprite3DInstanceData myInstanceData;
    bool myWarnedMissingTexture = false;
};
