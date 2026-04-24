
#include "MeshComponent.h"

#include "GameObject.h"

#include <CommonUtilities/Matrix4x4.hpp>
#include <tge/drawers/ModelDrawer.h>
#include <tge/engine.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/model/ModelFactory.h>
#include <tge/texture/TextureManager.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <iostream>
#include <vector>

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

    std::string GetBaseFileName(const std::string& aPath)
    {
        const size_t dotPos = aPath.find_last_of('.');
        const size_t slashPos = aPath.find_last_of("/\\");
        if (dotPos == std::string::npos)
        {
            return (slashPos == std::string::npos) ? aPath : aPath.substr(slashPos + 1);
        }

        const size_t startPos = (slashPos == std::string::npos) ? 0 : (slashPos + 1);
        return aPath.substr(startPos, dotPos - startPos);
    }

    std::string CombinePath(const std::string& aPrefix, const std::string& aName, const std::string& aSuffix)
    {
        return aPrefix + aName + aSuffix;
    }

    void AppendModelNameChannelCandidates(std::vector<std::string>& someCandidates,
        const std::string& aModelName,
        const std::string& aLowerSuffix,
        const std::string& aUpperSuffix)
    {
        if (aModelName.empty())
        {
            return;
        }

        // Keep one canonical texture location to avoid expensive multi-prefix probing.
        someCandidates.emplace_back(CombinePath("textures/", aModelName, aLowerSuffix));
        someCandidates.emplace_back(CombinePath("textures/", aModelName, aUpperSuffix));
    }

    const Tga::TextureResource* FindFirstTextureCandidate(Tga::TextureManager& aTextureManager,
        const std::vector<std::string>& someCandidates,
        const Tga::TextureSrgbMode aSrgbMode,
        std::string* outAppliedTexturePath = nullptr)
    {
        for (const std::string& candidate : someCandidates)
        {
            if (candidate.empty())
            {
                continue;
            }

            const Tga::TextureResource* texture = aTextureManager.TryGetTexture(candidate.c_str(), aSrgbMode);
            if (texture != nullptr)
            {
                if (outAppliedTexturePath)
                {
                    *outAppliedTexturePath = candidate;
                }
                return texture;
            }
        }

        if (outAppliedTexturePath)
        {
            outAppliedTexturePath->clear();
        }

        return nullptr;
    }

    bool ContainsCaseInsensitive(const std::string& aText, const std::string& aNeedle)
    {
        if (aNeedle.empty() || aNeedle.size() > aText.size())
        {
            return false;
        }

        return std::search(
            aText.begin(), aText.end(),
            aNeedle.begin(), aNeedle.end(),
            [](char lhs, char rhs)
            {
                return std::tolower(static_cast<unsigned char>(lhs)) == std::tolower(static_cast<unsigned char>(rhs));
            }) != aText.end();
    }

    bool UsesPointClampAtlasOverride(const std::string& aAtlasPath)
    {
        static const std::array<const char*, 2> kPointClampAtlasNames = {
            "example_atlas_path_1.dds",
            "example_atlas_path_2.dds"
        };

        for (const char* atlasName : kPointClampAtlasNames)
        {
            if (ContainsCaseInsensitive(aAtlasPath, atlasName))
            {
                return true;
            }
        }

        return false;
    }

    bool HasAnyTextureOverrides(const MeshTextureOverrides& someTextureOverrides)
    {
        for (int meshIndex = 0; meshIndex < MeshTextureOverrides::kMaxMeshCount; ++meshIndex)
        {
            for (int textureIndex = 0; textureIndex < MeshTextureOverrides::kTextureChannelCount; ++textureIndex)
            {
                if (!someTextureOverrides.textures[meshIndex][textureIndex].empty())
                {
                    return true;
                }
            }
        }

        return false;
    }

}

MeshComponent::MeshComponent(std::string aModelPath)
    : myModelPath(std::move(aModelPath))
{
}

MeshComponent::MeshComponent(const Tga::ModelInstance& aInstance)
    : myInstance(aInstance)
{
}

MeshComponent::MeshComponent(std::string aModelPath, const std::string& aVertexShaderPath, const std::string& aPixelShaderPath)
    : myModelPath(std::move(aModelPath))
{
    myModelShader.Init(aVertexShaderPath.c_str(), aPixelShaderPath.c_str());
}

MeshComponent::MeshComponent(const Tga::ModelInstance& aInstance, const std::string& aVertexShaderPath, const std::string& aPixelShaderPath)
    : myInstance(aInstance)
{
    myModelShader.Init(aVertexShaderPath.c_str(), aPixelShaderPath.c_str());
}

void MeshComponent::Init(Tga::Engine& /*anEngine*/)
{
    if (!myModelPath.empty() && !IsValid())
    {
        ReloadModel();
    }

    RefreshMaterialBindings();
}

void MeshComponent::RefreshMaterialBindings()
{
    if (!IsValid())
    {
        return;
    }

    auto model = myInstance.GetModel();
    if (!model)
    {
        return;
    }

    Tga::Engine& engine = *Tga::Engine::GetInstance();
    Tga::TextureManager& textureManager = engine.GetTextureManager();

    const std::string baseName = GetBaseFileName(myModelPath);
    const bool useAtlasOverrides = myUseAtlas;

    const std::string atlasAlbedo = useAtlasOverrides ? myAtlasTexture : std::string{};
    const std::string atlasNormal = useAtlasOverrides ? myAtlasNormalTexture : std::string{};
    const std::string atlasMaterial = useAtlasOverrides ? myAtlasMaterialTexture : std::string{};
    const std::string atlasFx = useAtlasOverrides ? myAtlasFxTexture : std::string{};

    // Atlas-specific sampler override for mesh rendering only.
    myForceNoMipAtlasSampler = useAtlasOverrides && !atlasAlbedo.empty() && UsesPointClampAtlasOverride(atlasAlbedo);

    const std::string ownerName = GetOwner() ? GetOwner()->GetName() : std::string("<no-owner>");


    for (int i = 0; i < static_cast<int>(model->GetMeshCount()); ++i)
    {
        const std::string materialName = model->GetMaterialName(i);
        const auto getTextureOverride = [&](int aTextureIndex) -> const std::string&
        {
            static const std::string emptyTexture;
            if (!myHasTextureOverrides || i >= MeshTextureOverrides::kMaxMeshCount)
            {
                return emptyTexture;
            }

            return myTextureOverrides.textures[i][aTextureIndex];
        };

        const std::string& overrideAlbedo = getTextureOverride(0);
        const std::string& overrideNormal = getTextureOverride(1);
        const std::string& overrideMaterial = getTextureOverride(2);
        const std::string& overrideFx = getTextureOverride(3);

        std::vector<std::string> albedoCandidates;
        if (!atlasAlbedo.empty())
        {
            albedoCandidates.push_back(atlasAlbedo);
        }
        if (!overrideAlbedo.empty())
        {
            albedoCandidates.push_back(overrideAlbedo);
        }
        AppendModelNameChannelCandidates(albedoCandidates, baseName, "_c.dds", "_C.dds");

        std::string appliedAlbedoPath;
        if (const Tga::TextureResource* albedo = FindFirstTextureCandidate(
            textureManager,
            albedoCandidates,
            Tga::TextureSrgbMode::ForceSrgbFormat,
            &appliedAlbedoPath))
        {
            myInstance.SetTexture(i, 0, albedo);
        }

        std::vector<std::string> normalCandidates;
        if (!atlasNormal.empty())
        {
            normalCandidates.push_back(atlasNormal);
        }
        if (!overrideNormal.empty())
        {
            normalCandidates.push_back(overrideNormal);
        }
        AppendModelNameChannelCandidates(normalCandidates, baseName, "_n.dds", "_N.dds");

        std::string appliedNormalPath;
        if (const Tga::TextureResource* normal = FindFirstTextureCandidate(
            textureManager,
            normalCandidates,
            Tga::TextureSrgbMode::ForceNoSrgbFormat,
            &appliedNormalPath))
        {
            myInstance.SetTexture(i, 1, normal);
        }

        std::vector<std::string> materialCandidates;
        if (!atlasMaterial.empty())
        {
            materialCandidates.push_back(atlasMaterial);
        }
        if (!overrideMaterial.empty())
        {
            materialCandidates.push_back(overrideMaterial);
        }
        AppendModelNameChannelCandidates(materialCandidates, baseName, "_m.dds", "_M.dds");

        std::string appliedMaterialPath;
        if (const Tga::TextureResource* material = FindFirstTextureCandidate(
            textureManager,
            materialCandidates,
            Tga::TextureSrgbMode::ForceNoSrgbFormat,
            &appliedMaterialPath))
        {
            myInstance.SetTexture(i, 2, material);
        }

        std::vector<std::string> fxCandidates;
        if (!atlasFx.empty())
        {
            fxCandidates.push_back(atlasFx);
        }
        if (!overrideFx.empty())
        {
            fxCandidates.push_back(overrideFx);
        }
        AppendModelNameChannelCandidates(fxCandidates, baseName, "_fx.dds", "_FX.dds");

        std::string appliedFxPath;
        if (const Tga::TextureResource* fx = FindFirstTextureCandidate(
            textureManager,
            fxCandidates,
            Tga::TextureSrgbMode::ForceNoSrgbFormat,
            &appliedFxPath))
        {
            myInstance.SetTexture(i, 3, fx);
        }

        const std::string albedoText = appliedAlbedoPath.empty() ? "<default>" : appliedAlbedoPath;
        const std::string normalText = appliedNormalPath.empty() ? "<default>" : appliedNormalPath;
        const std::string materialText = appliedMaterialPath.empty() ? "<default>" : appliedMaterialPath;
        const std::string fxText = appliedFxPath.empty() ? "<default>" : appliedFxPath;

        std::cout
            << "[MeshComponent] object='" << ownerName
            << "' model='" << myModelPath
            << "' mesh='" << materialName << "'"
            << " C=" << albedoText
            << " N=" << normalText
            << " M=" << materialText
            << " FX=" << fxText
            << "\n";
    }
}

void MeshComponent::Render()
{
    if (!IsValid())
    {
        return;
    }

    GameObject* owner = GetOwner();
    if (!owner)
    {
        return;
    }
    
    const auto worldMatrix = owner->GetTransform().GetWorldMatrix();
    myInstance.SetTransform(ToTgaMatrix(worldMatrix));

    auto& graphicsEngine = Tga::Engine::GetInstance()->GetGraphicsEngine();
    auto& modelDrawer = graphicsEngine.GetModelDrawer();
    Tga::GraphicsStateStack& graphicsStateStack = graphicsEngine.GetGraphicsStateStack();

    if (myForceNoMipAtlasSampler)
    {
        graphicsStateStack.Push();
        graphicsStateStack.SetSamplerState(Tga::SamplerFilter::Point, Tga::SamplerAddressMode::Clamp);
    }

    modelDrawer.DrawPbr(myInstance);

    if (myForceNoMipAtlasSampler)
    {
        graphicsStateStack.Pop();
    }
}

void MeshComponent::SetModelPath(const std::string& aModelPath)
{
    myModelPath = aModelPath;
}

bool MeshComponent::ReloadModel()
{
    if (myModelPath.empty())
    {
        return false;
    }

    Tga::ModelFactory& modelFactory = Tga::ModelFactory::GetInstance();
    if (!modelFactory.GetModel(myModelPath))
    {
        return false;
    }

    myInstance = modelFactory.GetModelInstance(myModelPath);
    RefreshMaterialBindings();
    return true;
}

const std::string& MeshComponent::GetModelPath() const
{
    return myModelPath;
}

void MeshComponent::SetModelInstance(const Tga::ModelInstance& aInstance)
{
    myInstance = aInstance;
}

Tga::ModelInstance& MeshComponent::GetModelInstance()
{
    return myInstance;
}

const Tga::ModelInstance& MeshComponent::GetModelInstance() const
{
    return myInstance;
}

void MeshComponent::SetCustomShader(const std::string& aVertexShaderPath, const std::string& aPixelShaderPath)
{
    myModelShader.Init(aVertexShaderPath.c_str(), aPixelShaderPath.c_str());
}

void MeshComponent::SetUseAtlas(bool aUseAtlas)
{
    myUseAtlas = aUseAtlas;
    RefreshMaterialBindings();
}

bool MeshComponent::GetUseAtlas() const
{
    return myUseAtlas;
}

void MeshComponent::SetTextureOverrides(const MeshTextureOverrides& someTextureOverrides)
{
    myTextureOverrides = someTextureOverrides;
    myHasTextureOverrides = HasAnyTextureOverrides(someTextureOverrides);
    RefreshMaterialBindings();
}

void MeshComponent::SetAtlasTexture(const std::string& aAtlasTexture)
{
    myAtlasTexture = aAtlasTexture;
    RefreshMaterialBindings();
}

const std::string& MeshComponent::GetAtlasTexture() const
{
    return myAtlasTexture;
}

void MeshComponent::SetAtlasNormalTexture(const std::string& aAtlasTexture)
{
    myAtlasNormalTexture = aAtlasTexture;
    RefreshMaterialBindings();
}

const std::string& MeshComponent::GetAtlasNormalTexture() const
{
    return myAtlasNormalTexture;
}

void MeshComponent::SetAtlasMaterialTexture(const std::string& aAtlasTexture)
{
    myAtlasMaterialTexture = aAtlasTexture;
    RefreshMaterialBindings();
}

const std::string& MeshComponent::GetAtlasMaterialTexture() const
{
    return myAtlasMaterialTexture;
}

void MeshComponent::SetAtlasFxTexture(const std::string& aAtlasTexture)
{
    myAtlasFxTexture = aAtlasTexture;
    RefreshMaterialBindings();
}

const std::string& MeshComponent::GetAtlasFxTexture() const
{
    return myAtlasFxTexture;
}

bool MeshComponent::IsValid() const
{
    return myInstance.GetModel() != nullptr;
}

void MeshComponent::SetVisible(bool aVisible)
{
    SetEnabled(aVisible);
}

bool MeshComponent::IsVisible() const
{
    return IsEnabled();
}

void MeshComponent::SetRenderMode(RenderMode /*aMode*/)
{
    myRenderMode = RenderMode::Pbr;
}

MeshComponent::RenderMode MeshComponent::GetRenderMode() const
{
    return myRenderMode;
}
