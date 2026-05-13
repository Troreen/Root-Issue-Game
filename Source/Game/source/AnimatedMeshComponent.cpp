#include "AnimatedMeshComponent.h"

#include "AnimationGraphComponent.h"
#include "GameObject.h"

#include <CommonUtilities/Matrix4x4.hpp>
#include <tge/animation/AnimationPlayer.h>
#include <tge/debug/LoadingProfiler.h>
#include <tge/drawers/ModelDrawer.h>
#include <tge/engine.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/model/ModelFactory.h>
#include <tge/texture/TextureManager.h>

#include <algorithm>
#include <vector>

using Matrix4x4f = CommonUtilities::Matrix4x4<float>;

namespace
{
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

        someCandidates.emplace_back(CombinePath("textures/", aModelName, aLowerSuffix));
        someCandidates.emplace_back(CombinePath("textures/", aModelName, aUpperSuffix));
    }

    // Texture candidates are ordered from explicit editor overrides to old filename conventions.
    // This lets animated meshes keep authored material overrides while still supporting legacy assets.
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
}

void AnimatedMeshComponent::RefreshMaterialBindings()
{
    if (!IsValid())
    {
        return;
    }

    auto model = GetModel();
    if (!model)
    {
        return;
    }

    Tga::TextureManager& textureManager = Tga::Engine::GetInstance()->GetTextureManager();
    const std::string baseName = GetBaseFileName(GetModelPath());

    GameObject* owner = GetOwner();
    const std::string ownerName = owner ? owner->GetName() : "unknown";

    if (!myHasTextureOverrides)
    {
        std::cout << "[AnimatedMeshComponent] object='" << ownerName
            << "' model='" << myModelPath
            << "' has no authored texture overrides; using model defaults.\n";
    }

    for (int i = 0; i < static_cast<int>(model->GetMeshCount()); ++i)
    {
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
        if (!overrideAlbedo.empty()) albedoCandidates.push_back(overrideAlbedo);
        AppendModelNameChannelCandidates(albedoCandidates, baseName, "_c.dds", "_C.dds");
        if (const Tga::TextureResource* albedo = FindFirstTextureCandidate(
            textureManager,
            albedoCandidates,
            Tga::TextureSrgbMode::ForceSrgbFormat))
        {
            myInstance.SetTexture(i, 0, albedo);
        }

        std::vector<std::string> normalCandidates;
        if (!overrideNormal.empty()) normalCandidates.push_back(overrideNormal);
        AppendModelNameChannelCandidates(normalCandidates, baseName, "_n.dds", "_N.dds");
        if (const Tga::TextureResource* normal = FindFirstTextureCandidate(
            textureManager,
            normalCandidates,
            Tga::TextureSrgbMode::ForceNoSrgbFormat))
        {
            myInstance.SetTexture(i, 1, normal);
        }

        std::vector<std::string> materialCandidates;
        if (!overrideMaterial.empty()) materialCandidates.push_back(overrideMaterial);
        AppendModelNameChannelCandidates(materialCandidates, baseName, "_m.dds", "_M.dds");
        if (const Tga::TextureResource* material = FindFirstTextureCandidate(
            textureManager,
            materialCandidates,
            Tga::TextureSrgbMode::ForceNoSrgbFormat))
        {
            myInstance.SetTexture(i, 2, material);
        }

        std::vector<std::string> fxCandidates;
        if (!overrideFx.empty()) fxCandidates.push_back(overrideFx);
        AppendModelNameChannelCandidates(fxCandidates, baseName, "_fx.dds", "_FX.dds");
        if (const Tga::TextureResource* fx = FindFirstTextureCandidate(
            textureManager,
            fxCandidates,
            Tga::TextureSrgbMode::ForceNoSrgbFormat))
        {
            myInstance.SetTexture(i, 3, fx);
        }
    }
}

AnimatedMeshComponent::AnimatedMeshComponent(std::string aModelPath)
    : myModelPath(std::move(aModelPath))
{
}

AnimatedMeshComponent::AnimatedMeshComponent(std::string aModelPath, const std::string& aVertexShaderPath, const std::string& aPixelShaderPath)
    : myModelPath(std::move(aModelPath))
{
    myModelShader.Init(aVertexShaderPath.c_str(), aPixelShaderPath.c_str());
}

void AnimatedMeshComponent::Init(Tga::Engine& /*anEngine*/)
{
    Tga::LoadingProfiler::Scope scope("AnimatedMeshComponent::Init");

    if (!myModelPath.empty() && !IsValid())
    {
        myInstance = Tga::ModelFactory::GetInstance().GetAnimatedModelInstance(myModelPath);
    }

    if (!IsValid())
    {
        return;
    }

    std::shared_ptr<Tga::Model> model = myInstance.GetModel();
    if (!model)
    {
        return;
    }
    RefreshMaterialBindings();
}

void AnimatedMeshComponent::Render()
{
    //std::cout << "Render: " << myInstance.GetTransform()(2,1) << std::endl;
    if (!IsValid())
    {
        return;
    }

    GameObject* owner = GetOwner();
    if (!owner)
    {
        return;
    }

    Matrix4x4f modelWorldMatrix = owner->GetTransform().GetWorldMatrix();
    if (!myFacingRight)
    {
        constexpr float kPiRadians = 2.0f; 
        modelWorldMatrix = Matrix4x4f::CreateRotationAroundY(kPiRadians) * modelWorldMatrix;
    }
    myInstance.SetTransform(ToTgaMatrix(modelWorldMatrix));

    auto& graphicsEngine = Tga::Engine::GetInstance()->GetGraphicsEngine();
    auto& modelDrawer = graphicsEngine.GetModelDrawer();

    modelDrawer.DrawPbr(myInstance);
}

void AnimatedMeshComponent::SetModelPath(const std::string& aModelPath)
{
    myModelPath = aModelPath;
}

const std::string& AnimatedMeshComponent::GetModelPath() const
{
    return myModelPath;
}

Tga::AnimatedModelInstance& AnimatedMeshComponent::GetModelInstance()
{
    return myInstance;
}

const Tga::AnimatedModelInstance& AnimatedMeshComponent::GetModelInstance() const
{
    return myInstance;
}

std::shared_ptr<Tga::Model> AnimatedMeshComponent::GetModel()
{
    return myInstance.GetModel();
}

void AnimatedMeshComponent::SetPose(const Tga::AnimationPlayer& anAnimationPlayer)
{
    myInstance.SetPose(anAnimationPlayer);
}

void AnimatedMeshComponent::SetPose(const Tga::LocalSpacePose& aPose)
{
    myInstance.SetPose(aPose);
}

void AnimatedMeshComponent::SetPose(const Tga::ModelSpacePose& aPose)
{
    myInstance.SetPose(aPose);
}

void AnimatedMeshComponent::ResetPose()
{
    myInstance.ResetPose();
}

bool AnimatedMeshComponent::SetAnimationFloat(const std::string& aParameterName, const float aValue)
{
    AnimationGraphComponent* graph = ResolveAnimationGraph();
    return graph ? graph->SetFloatParameter(aParameterName, aValue) : false;
}

bool AnimatedMeshComponent::SetAnimationInt(const std::string& aParameterName, const int aValue)
{
    AnimationGraphComponent* graph = ResolveAnimationGraph();
    return graph ? graph->SetIntParameter(aParameterName, aValue) : false;
}

bool AnimatedMeshComponent::SetAnimationBool(const std::string& aParameterName, const bool aValue)
{
    AnimationGraphComponent* graph = ResolveAnimationGraph();
    return graph ? graph->SetBoolParameter(aParameterName, aValue) : false;
}

bool AnimatedMeshComponent::SetAnimationVector3(const std::string& aParameterName, const Tga::Vector3f& aValue)
{
    AnimationGraphComponent* graph = ResolveAnimationGraph();
    return graph ? graph->SetVector3Parameter(aParameterName, aValue) : false;
}

bool AnimatedMeshComponent::SetAnimationString(const std::string& aParameterName, const std::string& aValue)
{
    AnimationGraphComponent* graph = ResolveAnimationGraph();
    return graph ? graph->SetStringParameter(aParameterName, aValue) : false;
}

bool AnimatedMeshComponent::SetClipWeightFromClipProperty(const std::string& aClipPropertyName, const float aWeight)
{
    return SetClipWeight(BuildWeightPropertyNameFromClipProperty(aClipPropertyName), aWeight);
}

bool AnimatedMeshComponent::SetClipWeight(const std::string& aWeightPropertyName, const float aWeight)
{
    if (aWeightPropertyName.empty())
    {
        return false;
    }

    const float clampedWeight = std::clamp(aWeight, 0.0f, 1.0f);
    return SetAnimationFloat(aWeightPropertyName, clampedWeight);
}

bool AnimatedMeshComponent::ActivateExclusiveClip(
    const std::vector<std::string>& someClipPropertyNames,
    const std::string& anActiveClipPropertyName)
{
    if (someClipPropertyNames.empty() || anActiveClipPropertyName.empty())
    {
        return false;
    }

    bool foundActiveClip = false;
    bool allWritesSucceeded = true;

    for (const std::string& clipPropertyName : someClipPropertyNames)
    {
        if (clipPropertyName.empty())
        {
            continue;
        }

        const bool isActiveClip = (clipPropertyName == anActiveClipPropertyName);
        foundActiveClip = foundActiveClip || isActiveClip;
        allWritesSucceeded = SetClipWeightFromClipProperty(clipPropertyName, isActiveClip ? 1.0f : 0.0f) && allWritesSucceeded;
    }

    return foundActiveClip && allWritesSucceeded;
}

bool AnimatedMeshComponent::SetAnimationSpeed(const float aSpeed)
{
    return SetAnimationFloat("anim_speed", (std::max)(0.0f, aSpeed));
}

AnimationGraphComponent* AnimatedMeshComponent::ResolveAnimationGraph()
{
    if (myAnimationGraph)
    {
        return myAnimationGraph;
    }

    GameObject* owner = GetOwner();
    if (!owner)
    {
        return nullptr;
    }

    myAnimationGraph = owner->GetComponent<AnimationGraphComponent>();
    return myAnimationGraph;
}

std::string AnimatedMeshComponent::BuildWeightPropertyNameFromClipProperty(const std::string& aClipPropertyName)
{
    if (aClipPropertyName.empty())
    {
        return std::string();
    }

    constexpr const char* clipPrefix = "clip_";
    if (aClipPropertyName.rfind(clipPrefix, 0) == 0)
    {
        return "w_" + aClipPropertyName.substr(5);
    }

    return "w_" + aClipPropertyName;
}

void AnimatedMeshComponent::SetCustomShader(const std::string& aVertexShaderPath, const std::string& aPixelShaderPath)
{
    myModelShader.Init(aVertexShaderPath.c_str(), aPixelShaderPath.c_str());
}

bool AnimatedMeshComponent::IsValid() const
{
    return myInstance.IsValid();
}

void AnimatedMeshComponent::SetVisible(bool aVisible)
{
    SetEnabled(aVisible);
}

bool AnimatedMeshComponent::IsVisible() const
{
    return IsEnabled();
}

void AnimatedMeshComponent::SetRenderMode(RenderMode /*aMode*/)
{
    myRenderMode = RenderMode::Pbr;
}

AnimatedMeshComponent::RenderMode AnimatedMeshComponent::GetRenderMode() const
{
    return myRenderMode;
}

void AnimatedMeshComponent::FlipY(bool aFacingRight)
{
    myFacingRight = aFacingRight;
}

void AnimatedMeshComponent::SetTextureOverrides(const MeshTextureOverrides& someTextureOverrides)
{
    myTextureOverrides = someTextureOverrides;
    myHasTextureOverrides = HasAnyTextureOverrides(someTextureOverrides);
    RefreshMaterialBindings();
}
