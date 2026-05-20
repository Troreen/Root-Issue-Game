#include "StaticSpriteComponent.h"

#include "GameObject.h"

#include <tge/drawers/SpriteDrawer.h>
#include <tge/engine.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/texture/TextureManager.h>

#include <CommonUtilities/Matrix4x4.hpp>

#include <algorithm>
#include <iostream>
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
}

void StaticSpriteComponent::Init(Tga::Engine& anEngine)
{
    if (mySpriteData.texturePath.empty())
    {
        return;
    }

    myTexture = anEngine.GetTextureManager().TryGetTexture(mySpriteData.texturePath.c_str());
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
