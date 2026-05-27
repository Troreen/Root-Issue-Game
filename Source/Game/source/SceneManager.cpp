#include "SceneManager.h"

#include <tge/settings/Settings.h>

#include <algorithm>
#include <filesystem>

namespace
{
    std::string GetStem(const std::filesystem::path& path)
    {
        return path.stem().string();
    }

    bool IsTgsFile(const std::filesystem::path& path)
    {
        return path.extension() == ".tgs";
    }

    bool IsSceneCachePath(const std::filesystem::path& path)
    {
        for (const std::filesystem::path& part : path)
        {
            if (part == ".scene_cache")
            {
                return true;
            }
        }

        return false;
    }
}

//SceneManager& SceneManager::GetInstance()
//{
//    static SceneManager instance;
//    return instance;
//}

void SceneManager::RefreshSceneList()
{
    myScenes.clear();

    std::filesystem::path root = Tga::Settings::GameAssetRoot();
    std::filesystem::path levelsPath = root / "Levels";

    if (!std::filesystem::exists(levelsPath))
    {
        return;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(levelsPath))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        const std::filesystem::path& path = entry.path();
        if (IsSceneCachePath(path) || !IsTgsFile(path))
        {
            continue;
        }

        std::filesystem::path relative = std::filesystem::relative(path, root);
        SceneEntry sceneEntry;
        sceneEntry.path = relative.generic_string();
        sceneEntry.name = GetStem(path);
        myScenes.push_back(sceneEntry);
    }

    std::sort(myScenes.begin(), myScenes.end(), [](const SceneEntry& a, const SceneEntry& b)
    {
        return a.name < b.name;
    });
}

const std::vector<SceneManager::SceneEntry>& SceneManager::GetScenes() const
{
    return myScenes;
}

void SceneManager::RequestScene(const std::string& aScenePath)
{
    myRequestedScene = aScenePath;
}

bool SceneManager::HasRequestedScene() const
{
    return !myRequestedScene.empty();
}

std::string SceneManager::ConsumeRequestedScene()
{
    std::string requested = myRequestedScene;
    myRequestedScene.clear();
    return requested;
}

void SceneManager::SetCurrentScene(const std::string& aScenePath)
{
    myCurrentScene = aScenePath;
}

const std::string& SceneManager::GetCurrentScene() const
{
    return myCurrentScene;
}
