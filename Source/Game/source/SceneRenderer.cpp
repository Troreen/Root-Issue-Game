#include "SceneRenderer.h"

#include "CameraSystem.h"
#include "DebugSettings.h"
#include "GameObject.h"
#include "MeshComponent.h"
#include "SpeechBubbleComponent.h"
#include "VfxSystem.h"

#include <CommonUtilities/Intersection.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <memory>
#include <string>

#include <tge/engine.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/graphics/DX11.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/model/Model.h>
#include "StaticSpriteComponent.h"


namespace
{
    struct ViewFrustum
    {
        Tga::Matrix4x4f viewMatrix;
        std::array<CommonUtilities::Plane<float>, 6> planes;
    };

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

    std::string NormalizeTagKey(const std::string& aTag)
    {
        std::string normalized;
        normalized.reserve(aTag.size());

        for (unsigned char c : aTag)
        {
            if (std::isalnum(c))
            {
                normalized.push_back(static_cast<char>(std::tolower(c)));
            }
        }

        return normalized;
    }

    bool IsTransparentRenderableObject(const GameObject& aObject)
    {
        const std::string tagKey = NormalizeTagKey(aObject.GetTag());
        return tagKey == "lvl1parallax" || tagKey == "lvl2parallax" || tagKey == "lvl2middleground" || tagKey == "lvl3parallax";
    }

    CommonUtilities::Vector3<float> ToCommonVector3(const Tga::Vector3f& aVector)
    {
        return { aVector.x, aVector.y, aVector.z };
    }

    ViewFrustum BuildViewFrustum(const Tga::Camera& aCamera)
    {
        ViewFrustum frustum;
        frustum.viewMatrix = aCamera.GetTransform().GetInverse();

        const Tga::Matrix4x4f& projection = aCamera.GetProjection();
        const float xScale = projection(1, 1);
        const float yScale = projection(2, 2);
        float nearPlane = 0.1f;
        float farPlane = 50000.0f;
        aCamera.GetProjectionPlanes(nearPlane, farPlane);

        frustum.planes[0].InitWithPointAndNormal({ 0.0f, 0.0f, 0.0f }, { -xScale, 0.0f, -1.0f });
        frustum.planes[1].InitWithPointAndNormal({ 0.0f, 0.0f, 0.0f }, { xScale, 0.0f, -1.0f });
        frustum.planes[2].InitWithPointAndNormal({ 0.0f, 0.0f, 0.0f }, { 0.0f, -yScale, -1.0f });
        frustum.planes[3].InitWithPointAndNormal({ 0.0f, 0.0f, 0.0f }, { 0.0f, yScale, -1.0f });
        frustum.planes[4].InitWithPointAndNormal({ 0.0f, 0.0f, nearPlane }, { 0.0f, 0.0f, -1.0f });
        frustum.planes[5].InitWithPointAndNormal({ 0.0f, 0.0f, farPlane }, { 0.0f, 0.0f, 1.0f });

        return frustum;
    }

    bool IsSphereVisibleInViewFrustum(const Tga::Vector3f& aWorldCenter, const float aWorldRadius, const ViewFrustum& aFrustum)
    {
        if (aWorldRadius <= 0.0f)
        {
            return true;
        }

        const Tga::Vector3f viewCenter = aWorldCenter * aFrustum.viewMatrix;
        const CommonUtilities::Vector3<float> viewCenterCommon = ToCommonVector3(viewCenter);

        for (const CommonUtilities::Plane<float>& plane : aFrustum.planes)
        {
            if (plane.GetDistanceToPoint(viewCenterCommon) > aWorldRadius)
            {
                return false;
            }
        }

        return true;
    }

    bool IsMeshVisibleInViewFrustum(GameObject& aObject, const MeshComponent& aMesh, const ViewFrustum& aFrustum)
    {
        const std::shared_ptr<Tga::Model> model = aMesh.GetModelInstance().GetModel();
        if (!model)
        {
            return true;
        }

        const Tga::Matrix4x4f worldMatrix = ToTgaMatrix(aObject.GetTransform().GetWorldMatrix());
        const float maxAxisScale = std::max({
            worldMatrix.GetRight().Length(),
            worldMatrix.GetUp().Length(),
            worldMatrix.GetForward().Length() });

        if (maxAxisScale <= 0.0f)
        {
            return true;
        }

        const std::vector<Tga::Model::MeshData>& meshDataList = model->GetMeshDataList();
        if (meshDataList.empty())
        {
            return true;
        }

        for (const Tga::Model::MeshData& meshData : meshDataList)
        {
            const Tga::Vector3f worldCenter = meshData.Bounds.Center * worldMatrix;
            const float worldRadius = meshData.Bounds.Radius * maxAxisScale;

            if (IsSphereVisibleInViewFrustum(worldCenter, worldRadius, aFrustum))
            {
                return true;
            }
        }

        return false;
    }
}

void SceneRenderer::Render(
    const std::vector<std::unique_ptr<GameObject>>& someObjects,
    CameraSystem& aCameraSystem,
    VfxSystem& aVfxSystem,
    bool aEnablePointLights,
    bool aEnableDirectionalLight,
    bool aEnableAmbientLight,
    bool aEnableFrustumCulling) const
{
    auto& engine = *Tga::Engine::GetInstance();
    auto& graphicsEngine = engine.GetGraphicsEngine();
    auto& graphicsStateStack = graphicsEngine.GetGraphicsStateStack();
    const float directionalYaw = GameDebugSettings::DirectionalLightYawDegrees();
    const float directionalPitch = GameDebugSettings::DirectionalLightPitchDegrees();
    const float directionalRoll = GameDebugSettings::DirectionalLightRollDegrees();
    const Tga::Color directionalColor = {
        GameDebugSettings::DirectionalLightColorR(),
        GameDebugSettings::DirectionalLightColorG(),
        GameDebugSettings::DirectionalLightColorB(),
        1.0f };
    const float directionalIntensity = GameDebugSettings::DirectionalLightIntensity();
    const Tga::Color ambientColor = {
        GameDebugSettings::AmbientLightColorR(),
        GameDebugSettings::AmbientLightColorG(),
        GameDebugSettings::AmbientLightColorB(),
        1.0f };
    const float ambientIntensity = GameDebugSettings::AmbientLightIntensity();
    const bool enableDirectionalLight = aEnableDirectionalLight && GameDebugSettings::EnableDirectionalLight();
    const bool enableAmbientLight = aEnableAmbientLight && GameDebugSettings::EnableAmbientLight();

    CommonUtilities::Camera3Df& camera = aCameraSystem.GetCamera();
    Tga::Camera& renderCamera = aCameraSystem.GetRenderCamera();

    Tga::DX11::BackBuffer->SetAsActiveTarget(Tga::DX11::DepthBuffer);

    renderCamera.SetTransform(ToTgaMatrix(camera.GetTransform().GetWorldMatrix()));
    graphicsStateStack.SetCamera(renderCamera);

    ViewFrustum viewFrustum;
    if (aEnableFrustumCulling)
    {
        viewFrustum = BuildViewFrustum(renderCamera);
    }

    if (enableDirectionalLight)
    {
        graphicsStateStack.SetDirectionalLight(Tga::DirectionalLight{
            Tga::Matrix4x4f::CreateFromRollPitchYaw(Tga::Rotator(directionalYaw, directionalPitch, directionalRoll)),
            directionalColor,
            directionalIntensity
            });
    }
    else
    {
        graphicsStateStack.SetDirectionalLight(Tga::DirectionalLight{});
    }

    if (enableAmbientLight)
    {
        const Tga::Color ambientLightColor = ambientIntensity * ambientColor;
        graphicsStateStack.SetAmbientLight(Tga::AmbientLight{
            ambientLightColor,
            Tga::AmbientLightType::Uniform,
            nullptr
            });
    }
    else
    {
        graphicsStateStack.SetAmbientLight(Tga::AmbientLight{ {} });
    }

    graphicsStateStack.ClearPointLights();

    // Checkpoint lights are intentionally disabled as per current art direction.
    (void)aEnablePointLights;

    std::vector<GameObject*> transparentObjects;
    transparentObjects.reserve(someObjects.size());

    graphicsStateStack.Push();
    graphicsStateStack.SetBlendState(Tga::BlendState::Disabled);
    graphicsStateStack.SetDepthStencilState(Tga::DepthStencilState::WriteLess);

    for (const auto& object : someObjects)
    {
        if (!object->IsActive())
        {
            continue;
        }

        MeshComponent* mesh = object->GetComponent<MeshComponent>();
        if (aEnableFrustumCulling && mesh && mesh->IsValid() && !IsMeshVisibleInViewFrustum(*object, *mesh, viewFrustum))
        {
            continue;
        }

        if (IsTransparentRenderableObject(*object))
        {
            transparentObjects.push_back(object.get());
            continue;
        }
    }

    for (const auto& object : someObjects)
    {
        if (!object->IsActive())
        {
            continue;
        }

        if (IsTransparentRenderableObject(*object))
        {
            continue;
        }

        MeshComponent* mesh = object->GetComponent<MeshComponent>();
        if (aEnableFrustumCulling && mesh && mesh->IsValid() && !IsMeshVisibleInViewFrustum(*object, *mesh, viewFrustum))
        {
            continue;
        }

        object->Render();
    }

    graphicsStateStack.SetBlendState(Tga::BlendState::AlphaBlend);
    graphicsStateStack.SetDepthStencilState(Tga::DepthStencilState::WriteLessOrEqual);
    graphicsStateStack.SetDepthStencilState(Tga::DepthStencilState::ReadOnlyLessOrEqual);
    graphicsStateStack.SetRasterizerState(Tga::RasterizerState::NoFaceCulling);

    std::stable_sort(
        transparentObjects.begin(),
        transparentObjects.end(),
        [](const GameObject* aLeft, const GameObject* aRight)
        {
            return aLeft->GetTransform().GetPosition().z > aRight->GetTransform().GetPosition().z;
        });

    for (GameObject* object : transparentObjects)
    {
        object->Render();
    }

    graphicsStateStack.Pop();

    aVfxSystem.Render();

    SpeechBubbleComponent::FlushQueuedRender();
}
