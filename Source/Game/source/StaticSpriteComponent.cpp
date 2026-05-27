#include "StaticSpriteComponent.h"

#include "GameObject.h"

#include <tge/drawers/SpriteDrawer.h>
#include <tge/engine.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/texture/texture.h>
#include <tge/texture/TextureManager.h>
#include <tge/shaders/SpriteShader.h>

#include <CommonUtilities/Matrix4x4.hpp>

#include <algorithm>
#include <iostream>
#include <memory>
#include <utility>

namespace
{
    Tga::Matrix4x4f ToTgaMatrix(const CommonUtilities::Matrix4x4<float>& aMatrix)
    {
        Tga::Matrix4x4f result;
        for (int r = 1; r < 5; ++r)
        {
            for (int c = 1; c < 5; ++c)
            {
                result(r, c) = aMatrix(r, c);
            }
        }
        return result;
    }

    std::string NormalizeAssetPath(std::string aPath)
    {
        std::replace(aPath.begin(), aPath.end(), '\\', '/');
        return aPath;
    }
}

StaticSpriteComponent::StaticSpriteComponent(SceneSpriteData someSpriteData)
    : mySpriteData(std::move(someSpriteData))
{
    mySpriteData.texturePath = NormalizeAssetPath(mySpriteData.texturePath);
    for (std::string& texturePath : mySpriteData.texturePaths)
    {
        texturePath = NormalizeAssetPath(texturePath);
    }

    if (mySpriteData.texturePaths[0].empty())
    {
        mySpriteData.texturePaths[0] = mySpriteData.texturePath;
    }
    else
    {
        mySpriteData.texturePath = mySpriteData.texturePaths[0];
    }
}

StaticSpriteComponent::~StaticSpriteComponent() = default;

void StaticSpriteComponent::Init(Tga::Engine& anEngine)
{
    if (mySpriteData.texturePath.empty())
    {
        return;
    }

    Tga::TextureManager& textureManager = anEngine.GetTextureManager();
    myTexture = textureManager.TryGetTexture(
        mySpriteData.texturePath.c_str(),
        Tga::TextureSrgbMode::ForceSrgbFormat);
    if (!myTexture)
    {
        if (!myWarnedMissingTexture)
        {
            std::cout << "[StaticSprite] Missing texture: " << mySpriteData.texturePath << "\n";
            myWarnedMissingTexture = true;
        }
        return;
    }

    mySharedData.myTexture = myTexture;
    myHasMaterialMaps = false;

    constexpr int kSpriteMapTextureOffset = 1;
    for (int mapIndex = 0; mapIndex < Tga::MAP_MAX; ++mapIndex)
    {
        const int textureIndex = kSpriteMapTextureOffset + mapIndex;
        const std::string& mapPath = mySpriteData.texturePaths[textureIndex];
        if (mapPath.empty())
        {
            myMapTextures[mapIndex] = nullptr;
            mySharedData.myMaps[mapIndex] = nullptr;
            continue;
        }

        myMapTextures[mapIndex] = textureManager.TryGetTexture(
            mapPath.c_str(),
            Tga::TextureSrgbMode::ForceNoSrgbFormat);
        mySharedData.myMaps[mapIndex] = myMapTextures[mapIndex];
        myHasMaterialMaps = myHasMaterialMaps || myMapTextures[mapIndex] != nullptr;
    }

    if (myHasMaterialMaps && !myPbrSpriteShaderInitialized)
    {
        myPbrSpriteShader = std::make_unique<Tga::SpriteShader>();
        myPbrSpriteShaderInitialized = myPbrSpriteShader->Init(
            "shaders/instanced_sprite_shader_VS",
            "shaders/PbrModelShaderPS");
    }

    mySharedData.myCustomShader = myHasMaterialMaps && myPbrSpriteShaderInitialized
        ? myPbrSpriteShader.get()
        : nullptr;
    myWarnedMissingTexture = false;
}

void StaticSpriteComponent::Render()
{
    if (!myTexture)
    {
        return;
    }

    GameObject* owner = GetOwner();
    Tga::Engine* engine = Tga::Engine::GetInstance();
    if (!owner || !engine)
    {
        return;
    }

    const float sizeX = (std::max)(0.01f, mySpriteData.size.x);
    const float sizeY = (std::max)(0.01f, mySpriteData.size.y);

    Tga::Matrix4x4f localTransform = Tga::Matrix4x4f::CreateFromScale({ sizeX, sizeY, 1.0f });
    localTransform.SetPosition({
        -mySpriteData.pivot.x * sizeX,
        mySpriteData.pivot.y * sizeY,
        0.0f
        });

    myInstanceData.myTransform = localTransform * ToTgaMatrix(owner->GetTransform().GetWorldMatrix());

    //Tga::GraphicsStateStack& graphicsStateStack = engine->GetGraphicsEngine().GetGraphicsStateStack();
    //graphicsStateStack.Push();
    //graphicsStateStack.SetBlendState(Tga::BlendState::AlphaBlend);
    //graphicsStateStack.SetDepthStencilState(Tga::DepthStencilState::ReadOnlyLessOrEqual);
    //graphicsStateStack.SetRasterizerState(Tga::RasterizerState::NoFaceCulling);
    engine->GetGraphicsEngine().GetSpriteDrawer().Draw(mySharedData, myInstanceData);
    //graphicsStateStack.Pop();
}
