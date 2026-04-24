#include "Options.h"
#include <tge/animation/Script/AnimationNodes.h>
#include <tge/script/Nodes/CommonMathNodes.h>
#include <tge/script/Nodes/CommonNodes.h>

#include "MeshComponent.h"
#include "GameObject.h"

namespace
{
    void RegisterAnimationGraphNodesOnce()
    {
        static bool isRegistered = false;
        if (isRegistered)
        {
            return;
        }

        Tga::RegisterCommonNodes();
        Tga::RegisterCommonMathNodes();
        Tga::RegisterAnimationNodes();

        isRegistered = true;
    }

    bool gEnableFrustumCulling = true;

    void DumpSceneVisibilitySnapshot(
        const std::vector<std::unique_ptr<GameObject>>& someObjects,
        const CameraSystem& aCameraSystem)
    {
        const CommonUtilities::Camera3Df& camera = aCameraSystem.GetCamera();
        const auto cameraPosition = camera.GetTransform().GetPosition();
        const auto cameraForward = camera.GetTransform().GetForward();

        size_t activeObjectCount = 0;
        size_t meshComponentCount = 0;
        size_t validMeshComponentCount = 0;
        size_t meshDefaultCount = 0;
        size_t meshLambertCount = 0;
        size_t meshPbrCount = 0;
        size_t meshCustomCount = 0;

        float nearestDistance = (std::numeric_limits<float>::max)();
        float farthestDistance = 0.0f;

        for (const std::unique_ptr<GameObject>& object : someObjects)
        {
            if (!object || !object->IsActive())
            {
                continue;
            }

            ++activeObjectCount;
            const float distanceToCamera = (object->GetTransform().GetPosition() - cameraPosition).Length();
            nearestDistance = (std::min)(nearestDistance, distanceToCamera);
            farthestDistance = (std::max)(farthestDistance, distanceToCamera);

            if (MeshComponent* mesh = object->GetComponent<MeshComponent>())
            {
                ++meshComponentCount;
                if (mesh->IsValid())
                {
                    ++validMeshComponentCount;

                    switch (mesh->GetRenderMode())
                    {
                    case MeshComponent::RenderMode::Lambert:
                        ++meshLambertCount;
                        break;
                    case MeshComponent::RenderMode::Pbr:
                        ++meshPbrCount;
                        break;
                    case MeshComponent::RenderMode::Custom:
                        ++meshCustomCount;
                        break;
                    case MeshComponent::RenderMode::Default:
                    default:
                        ++meshDefaultCount;
                        break;
                    }
                }
            }
        }

        if (activeObjectCount == 0)
        {
            nearestDistance = 0.0f;
        }

        std::cout << "[RenderDebug] cameraPos=(" << cameraPosition.x << ", " << cameraPosition.y << ", " << cameraPosition.z
            << ") cameraForward=(" << cameraForward.x << ", " << cameraForward.y << ", " << cameraForward.z << ")"
            << " near=" << camera.GetNearPlane()
            << " far=" << camera.GetFarPlane() << "\n";

        std::cout << "[RenderDebug] activeObjects=" << activeObjectCount
            << " meshComponents=" << meshComponentCount
            << " validMeshes=" << validMeshComponentCount
            << " renderModes(default/lambert/pbr/custom)="
            << meshDefaultCount << "/" << meshLambertCount << "/" << meshPbrCount << "/" << meshCustomCount
            << " nearestObjectDist=" << nearestDistance
            << " farthestObjectDist=" << farthestDistance
            << " frustumCulling=" << (gEnableFrustumCulling ? "ON" : "OFF") << "\n";

        if (activeObjectCount > 0 && nearestDistance < camera.GetNearPlane())
        {
            std::cout << "[RenderDebug] WARNING: nearest object is in front of near plane and may be clipped."
                << " Lower near plane or move camera back.\n";
        }
    }
}


void Options::Init(CameraSystem& aCamera, const char* argv[])
{
	UNREFERENCED_PARAMETER(argv);
	myCameraSystem = &aCamera;
}

eState Options::Update()
{

	return eState::COUNT;
}

void Options::Render()
{

}