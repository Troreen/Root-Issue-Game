#pragma once

#include "SceneImportService.h"
#include "SceneObjectData.h"

#include <tge/debug/LoadingProfiler.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

class GameObject;

// Plain scene data. Safe to produce from the async path because it contains no
// live gameplay, render, GPU, script, or component objects.
struct LoadedSceneData
{
    std::string scenePath;
    std::vector<SceneObjectData> objects;
};

// Live scene objects. These must be built/applied on the main thread unless the
// engine proves a specific subsystem is thread-safe.
struct LoadedSceneObjects
{
    std::string scenePath;
    std::vector<std::unique_ptr<GameObject>> objects;
};

class SceneLoadingService final
{
public:
    // Backend facade used by SceneTransitionController. Keep orchestration out of
    // this class; it should only parse/cache scene data and build GameObjects when
    // explicitly asked to do so.
    static void BeginSceneLoad(const std::string& aScenePath)
    {
        Tga::LoadingProfiler::GetInstance().BeginSceneLoad(aScenePath);
    }

    static std::vector<SceneObjectData> LoadSceneObjects(const std::string& aScenePath)
    {
        return LoadSceneDataAsyncSafe(aScenePath).objects;
    }

    // The only loading work allowed on a background thread during transitions or preload.
    static LoadedSceneData LoadSceneDataAsyncSafe(const std::string& aScenePath)
    {
        SceneImportService importer;
        LoadedSceneData result;
        result.scenePath = aScenePath;
        result.objects = importer.LoadSceneObjects(aScenePath);
        return result;
    }

    // Main-thread construction step. Factories/components may touch render, script,
    // audio, or gameplay systems, so the controller only calls this at black.
    static std::vector<std::unique_ptr<GameObject>> BuildGameObjects(
        const std::vector<SceneObjectData>& someSceneObjects)
    {
        SceneImportService importer;
        return importer.BuildGameObjects(someSceneObjects);
    }

    // Kept for boot/debug fallbacks. Ordinary gameplay transitions should go through
    // SceneTransitionController so fade/queue/preload behavior stays central.
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
