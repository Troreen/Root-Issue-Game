#pragma once

#include "Component.h"
#include "ModelTextureOverrides.h"

#include <string>
#include <tge/model/ModelInstance.h>
#include <tge/shaders/ModelShader.h>

namespace Tga
{
    class Engine;
}

/// Component that renders a 3D model using TGE's ModelDrawer.
class MeshComponent final : public Component
{
public:
    enum class RenderMode
    {
        Default,
        Lambert,
        Pbr,
        Custom
    };

    MeshComponent() = default;
    explicit MeshComponent(std::string aModelPath);
    explicit MeshComponent(const Tga::ModelInstance& aInstance);
    explicit MeshComponent(std::string aModelPath, const std::string& aVertexShaderPath, const std::string& aPixelShaderPath);
    explicit MeshComponent(const Tga::ModelInstance& aInstance, const std::string& aVertexShaderPath, const std::string& aPixelShaderPath);

    void Init(Tga::Engine& anEngine) override;
    void Render() override;

    void SetModelPath(const std::string& aModelPath);
    bool ReloadModel();
    const std::string& GetModelPath() const;

    void SetModelInstance(const Tga::ModelInstance& aInstance);
    Tga::ModelInstance& GetModelInstance();
    const Tga::ModelInstance& GetModelInstance() const;

    void SetCustomShader(const std::string& aVertexShaderPath, const std::string& aPixelShaderPath);

    void SetUseAtlas(bool aUseAtlas);
    bool GetUseAtlas() const;
    void SetTextureOverrides(const MeshTextureOverrides& someTextureOverrides);
    void SetAtlasTexture(const std::string& aAtlasTexture);
    const std::string& GetAtlasTexture() const;
    void SetAtlasNormalTexture(const std::string& aAtlasTexture);
    const std::string& GetAtlasNormalTexture() const;
    void SetAtlasMaterialTexture(const std::string& aAtlasTexture);
    const std::string& GetAtlasMaterialTexture() const;
    void SetAtlasFxTexture(const std::string& aAtlasTexture);
    const std::string& GetAtlasFxTexture() const;

    bool IsValid() const;

    void SetVisible(bool aVisible);
    bool IsVisible() const;

    void SetRenderMode(RenderMode aMode);
    RenderMode GetRenderMode() const;

private:
    void RefreshMaterialBindings();

    std::string myModelPath;
    Tga::ModelInstance myInstance;
    RenderMode myRenderMode = RenderMode::Pbr;

    Tga::ModelShader myModelShader;

    bool myUseAtlas = false;
    bool myForceNoMipAtlasSampler = false;
    bool myHasTextureOverrides = false;
    MeshTextureOverrides myTextureOverrides;
    std::string myAtlasTexture;
    std::string myAtlasNormalTexture;
    std::string myAtlasMaterialTexture;
    std::string myAtlasFxTexture;
};
