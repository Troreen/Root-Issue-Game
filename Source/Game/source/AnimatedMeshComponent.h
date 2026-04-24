#pragma once

#include "Component.h"

#include <string>
#include <vector>
#include <tge/math/Vector3.h>
#include <tge/model/AnimatedModelInstance.h>
#include <tge/shaders/ModelShader.h>

namespace Tga
{
    class Engine;
    class AnimationPlayer;
    struct LocalSpacePose;
    struct ModelSpacePose;
}

class AnimationGraphComponent;

/// Component that renders a skinned/animated 3D model using TGE's ModelDrawer.
/// Animation playback is driven externally via SetPose().
class AnimatedMeshComponent final : public Component
{
public:
    enum class RenderMode
    {
        Default,
        Lambert,
        Pbr,
        Custom
    };

    AnimatedMeshComponent() = default;
    explicit AnimatedMeshComponent(std::string aModelPath);
    explicit AnimatedMeshComponent(std::string aModelPath, const std::string& aVertexShaderPath, const std::string& aPixelShaderPath);

    void Init(Tga::Engine& anEngine) override;
    void Render() override;

    void SetModelPath(const std::string& aModelPath);
    const std::string& GetModelPath() const;

    Tga::AnimatedModelInstance& GetModelInstance();
    const Tga::AnimatedModelInstance& GetModelInstance() const;

    /// Returns the underlying Model (needed to create AnimationPlayers).
    std::shared_ptr<Tga::Model> GetModel();

    /// Apply a pose from an AnimationPlayer. Call this every frame before Render().
    void SetPose(const Tga::AnimationPlayer& anAnimationPlayer);
    void SetPose(const Tga::LocalSpacePose& aPose);
    void SetPose(const Tga::ModelSpacePose& aPose);
    void ResetPose();

    /// Convenience wrappers that forward parameters to AnimationGraphComponent on the same GameObject.
    /// These return false when no AnimationGraphComponent is attached.
    bool SetAnimationFloat(const std::string& aParameterName, float aValue);
    bool SetAnimationInt(const std::string& aParameterName, int aValue);
    bool SetAnimationBool(const std::string& aParameterName, bool aValue);
    bool SetAnimationVector3(const std::string& aParameterName, const Tga::Vector3f& aValue);
    bool SetAnimationString(const std::string& aParameterName, const std::string& aValue);

    /// Writes a blend weight using the graph naming convention (clip_run -> w_run).
    bool SetClipWeightFromClipProperty(const std::string& aClipPropertyName, float aWeight);

    /// Writes a blend weight directly to a named weight parameter (for example w_run).
    bool SetClipWeight(const std::string& aWeightPropertyName, float aWeight);

    /// Activates one clip and clears all others in the provided list.
    bool ActivateExclusiveClip(
        const std::vector<std::string>& someClipPropertyNames,
        const std::string& anActiveClipPropertyName);

    /// Shortcut for the common playback speed parameter.
    bool SetAnimationSpeed(float aSpeed);

    void SetCustomShader(const std::string& aVertexShaderPath, const std::string& aPixelShaderPath);

    bool IsValid() const;

    void SetVisible(bool aVisible);
    bool IsVisible() const;

    void SetRenderMode(RenderMode aMode);
    RenderMode GetRenderMode() const;

    void FlipY(bool aFacingRight);

private:
    AnimationGraphComponent* ResolveAnimationGraph();
    static std::string BuildWeightPropertyNameFromClipProperty(const std::string& aClipPropertyName);

    std::string myModelPath;
    Tga::AnimatedModelInstance myInstance;
    RenderMode myRenderMode = RenderMode::Pbr;
    bool myFacingRight = true;
    AnimationGraphComponent* myAnimationGraph = nullptr;

    Tga::ModelShader myModelShader;
};
