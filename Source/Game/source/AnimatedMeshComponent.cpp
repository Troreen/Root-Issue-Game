#include "AnimatedMeshComponent.h"

#include "AnimationGraphComponent.h"
#include "GameObject.h"

#include <CommonUtilities/Matrix4x4.hpp>
#include <tge/animation/AnimationPlayer.h>
#include <tge/drawers/ModelDrawer.h>
#include <tge/engine.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/model/ModelFactory.h>
#include <tge/texture/TextureManager.h>

#include <algorithm>

using Matrix4x4f = CommonUtilities::Matrix4x4<float>;

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

    Tga::TextureManager& textureManager = Tga::Engine::GetInstance()->GetTextureManager();
    const std::string baseName = GetBaseFileName(myModelPath);
    const std::string texturePath = "textures/Common/" + baseName + "_T.dds";

    const Tga::TextureResource* albedo = textureManager.TryGetTexture(texturePath.c_str());
    if (!albedo)
    {
        return;
    }

    for (int i = 0; i < static_cast<int>(model->GetMeshCount()); ++i)
    {
        myInstance.SetTexture(i, 0, albedo);
    }
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
