#pragma once

#include <memory>
#include <string>
#include <vector>

class GameObject;
struct SceneObjectData;

class SceneImportService
{
public:
    std::vector<SceneObjectData> LoadSceneObjects(const std::string& scenePath) const;

    std::vector<std::unique_ptr<GameObject>> BuildGameObjects(
        const std::string& scenePath) const;

    std::vector<std::unique_ptr<GameObject>> BuildGameObjects(
        const std::vector<SceneObjectData>& someSceneObjects) const;
};
