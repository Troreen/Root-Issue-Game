#pragma once

#include <string>
#include <vector>

class SceneManager
{
public:
    struct SceneEntry
    {
        std::string name;
        std::string path;
    };
    
    SceneManager() = default;

    SceneManager(const SceneManager& aSceneManager) = delete;
    SceneManager& operator=(const SceneManager& aSceneManager) = delete;

    //static SceneManager& GetInstance();

    void RefreshSceneList();
    const std::vector<SceneEntry>& GetScenes() const;

    void RequestScene(const std::string& aScenePath);
    bool HasRequestedScene() const;
    std::string ConsumeRequestedScene();

    void SetCurrentScene(const std::string& aScenePath);
    const std::string& GetCurrentScene() const;

private:

    std::vector<SceneEntry> myScenes;
    std::string myRequestedScene;
    std::string myCurrentScene;
};
