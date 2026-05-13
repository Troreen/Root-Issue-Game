#pragma once

#include "SceneImportService.h"
#include "SceneObjectData.h"

#include <tge/debug/LoadingProfiler.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

class GameObject;

struct LoadedSceneData
{
    std::string scenePath;
    std::vector<SceneObjectData> objects;
};

struct LoadedSceneObjects
{
    std::string scenePath;
    std::vector<std::unique_ptr<GameObject>> objects;
};

class SceneLoadingService final
{
public:
    static void BeginSceneLoad(const std::string& aScenePath)
    {
        Tga::LoadingProfiler::GetInstance().BeginSceneLoad(aScenePath);
    }

    static std::vector<SceneObjectData> LoadSceneObjects(const std::string& aScenePath)
    {
        return LoadSceneDataAsyncSafe(aScenePath).objects;
    }

    static LoadedSceneData LoadSceneDataAsyncSafe(const std::string& aScenePath)
    {
        SceneImportService importer;
        LoadedSceneData result;
        result.scenePath = aScenePath;
        result.objects = importer.LoadSceneObjects(aScenePath);
        return result;
    }

    static std::vector<std::unique_ptr<GameObject>> BuildGameObjects(
        const std::vector<SceneObjectData>& someSceneObjects)
    {
        SceneImportService importer;
        return importer.BuildGameObjects(someSceneObjects);
    }

    static LoadedSceneObjects LoadSceneSynchronously(const std::string& aScenePath)
    {
        BeginSceneLoad(aScenePath);

        LoadedSceneData sceneData = LoadSceneDataAsyncSafe(aScenePath);
        LoadedSceneObjects result;
        result.scenePath = sceneData.scenePath;
        result.objects = BuildGameObjects(sceneData.objects);

        MarkSceneObjectsReady();
        return result;
    }

    static void MarkSceneObjectsReady()
    {
        Tga::LoadingProfiler::GetInstance().MarkAsyncLoadFinished();
    }

    static void FinishSceneApply()
    {
        Tga::LoadingProfiler::GetInstance().FinishSceneLoadAndPrint();
    }

private:
    SceneLoadingService() = delete;
};
