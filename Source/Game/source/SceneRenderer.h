#pragma once

#include <memory>
#include <vector>

class CameraSystem;
class GameObject;
class VfxSystem;

class SceneRenderer
{
public:
    void Render(
        const std::vector<std::unique_ptr<GameObject>>& someObjects,
        CameraSystem& aCameraSystem,
        VfxSystem& aVfxSystem,
        bool aEnablePointLights,
        bool aEnableDirectionalLight,
        bool aEnableAmbientLight,
        bool aEnableFrustumCulling) const;
};
