#include "SceneTransitionController.h"

#include "GameObject.h"
#include "WorldTransitionService.h"

#include <tge/debug/LoadingProfiler.h>
#include <tge/settings/Settings.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <unordered_set>
#include <utility>

namespace
{
    constexpr float kMinimumFadeSeconds = 0.01f;

    // Passive preload should never point back at the scene we are already in.
    // Same-scene reloads can still be requested explicitly with forceReload.
    bool IsValidPreloadTarget(const std::string& aCandidate, const std::string& aCurrentScene)
    {
        return !aCandidate.empty() && aCandidate != aCurrentScene;
    }
}

void SceneTransitionController::Initialize(
    ApplySceneCallback anApplySceneCallback,
    SceneTransitionCallback aSceneTransitionCallback,
    CurrentSceneCallback aCurrentSceneCallback)
{
    myApplySceneCallback = std::move(anApplySceneCallback);
    mySceneTransitionCallback = std::move(aSceneTransitionCallback);
    myCurrentSceneCallback = std::move(aCurrentSceneCallback);
}

void SceneTransitionController::Shutdown()
{
    CancelPendingWork();
    myApplySceneCallback = {};
    mySceneTransitionCallback = {};
    myCurrentSceneCallback = {};
}

bool SceneTransitionController::LoadBootScene(const std::string& aScenePath)
{
    const std::string resolvedScenePath = ResolveScenePath(aScenePath);
    if (resolvedScenePath.empty() || !myApplySceneCallback)
    {
        return false;
    }

    CancelPendingWork();

    Tga::LoadingProfiler::GetInstance().BeginSceneLoad(resolvedScenePath);
    LoadedSceneData sceneData = SceneLoadingService::LoadSceneDataAsyncSafe(resolvedScenePath);
    LoadedSceneObjects loadedObjects;
    loadedObjects.scenePath = sceneData.scenePath;
    loadedObjects.objects = SceneLoadingService::BuildGameObjects(sceneData.objects);
    SceneLoadingService::MarkSceneObjectsReady();
    myApplySceneCallback(std::move(loadedObjects.objects), loadedObjects.scenePath, "");

    StartPreloadFromSceneData(sceneData.scenePath, sceneData.objects);
    return true;
}

// -----------------------------------------------------------------------------
// Runtime requests
// -----------------------------------------------------------------------------
// Runtime transitions are funneled through this method so fade, queueing, preload
// reuse, and profiler boundaries stay consistent.
bool SceneTransitionController::RequestTransition(const Request& aRequest)
{
    Request request = aRequest;
    request.targetScene = ResolveScenePath(request.targetScene);
    request.fadeSeconds = (std::max)(kMinimumFadeSeconds, request.fadeSeconds);

    if (request.targetScene.empty() || !myApplySceneCallback)
    {
        return false;
    }

    const std::string currentScene = myCurrentSceneCallback ? myCurrentSceneCallback() : std::string{};
    if (!request.forceReload && request.targetScene == currentScene)
    {
        return false;
    }

    if (myState == State::PreparingRequestedScene
        || myState == State::FadingOut
        || myState == State::ApplyingLoadedScene
        || myState == State::FadingIn)
    {
        // Linear progression only needs one replacement request. Repeated trigger
        // spam should not create a backlog of obsolete transitions.
        QueueRequest(request);
        return true;
    }

    myActiveRequest = request;
    myFadeSeconds = request.fadeSeconds;
    myFadeAlpha = 0.0f;

    Tga::LoadingProfiler::GetInstance().BeginSceneLoad(request.targetScene);
    if (TryUseReadyPreload(request))
    {
        return true;
    }

    if (TryAttachPendingPreload(request))
    {
        return true;
    }

    StartRequestedScenePrepare(request);
    return true;
}

void SceneTransitionController::Update(const float aDeltaTime)
{
    PollPassivePreload();

    if (myState == State::PreparingRequestedScene)
    {
        if (!IsFutureReady(myRequestedSceneFuture))
        {
            return;
        }

        LoadedSceneData sceneData = myRequestedSceneFuture.get();
        // Data is ready, but GameObjects still are not built here. The fade-out
        // gives us a full-black point to do main-thread construction safely.
        SceneLoadingService::MarkSceneObjectsReady();
        StartFadeOutWithPreparedData({
            sceneData.scenePath,
            myActiveRequest.targetSpawnId,
            std::move(sceneData.objects)
            });
        return;
    }

    if (myState == State::FadingOut)
    {
        myFadeAlpha = (std::min)(1.0f, myFadeAlpha + aDeltaTime / myFadeSeconds);
        if (myFadeAlpha >= 1.0f)
        {
            // All visible scene replacement happens behind the black overlay.
            ApplyPreparedScene();
        }
        return;
    }

    if (myState == State::FadingIn)
    {
        myFadeAlpha = (std::max)(0.0f, myFadeAlpha - aDeltaTime / myFadeSeconds);
        if (myFadeAlpha <= 0.0f)
        {
            FinishFadeIn();
        }
    }
}

void SceneTransitionController::CancelPendingWork()
{
    if (myRequestedSceneFuture.valid())
    {
        myRequestedSceneFuture.wait();
    }

    if (myPreloadFuture.valid())
    {
        myPreloadFuture.wait();
    }

    // std::future does not support cancellation. Waiting above keeps ownership
    // simple and prevents a worker from outliving the gameplay state it loaded for.
    myState = State::Idle;
    myHasQueuedRequest = false;
    myHasPreparedSceneData = false;
    myHasPreloadedSceneData = false;
    myPreloadScenePath.clear();
    myFadeAlpha = 0.0f;
}

bool SceneTransitionController::IsTransitionActive() const
{
    return myState == State::PreparingRequestedScene
        || myState == State::FadingOut
        || myState == State::ApplyingLoadedScene
        || myState == State::FadingIn;
}

float SceneTransitionController::GetFadeAlpha() const
{
    return myFadeAlpha;
}

std::string SceneTransitionController::ResolveScenePath(const std::string& aScenePath)
{
    if (aScenePath.empty())
    {
        return {};
    }

    namespace fs = std::filesystem;
    const fs::path authoredPath(aScenePath);
    std::vector<fs::path> candidates;
    candidates.reserve(4);

    candidates.push_back(authoredPath);
    if (authoredPath.extension() != ".tgs")
    {
        candidates.push_back(authoredPath.string() + ".tgs");
    }

    if (!authoredPath.has_parent_path())
    {
        candidates.push_back(fs::path("Levels") / authoredPath);
        if (authoredPath.extension() != ".tgs")
        {
            candidates.push_back(fs::path("Levels") / (authoredPath.string() + ".tgs"));
        }
    }

    for (const fs::path& candidate : candidates)
    {
        const std::string candidateString = candidate.generic_string();
        if (!Tga::Settings::ResolveAssetPath(candidateString).empty())
        {
            return candidateString;
        }

        if (fs::exists(candidate))
        {
            return candidateString;
        }

        const fs::path rootedPath = Tga::Settings::GameAssetRoot() / candidate;
        if (fs::exists(rootedPath))
        {
            return candidateString;
        }
    }

    std::cout << "[SceneTransition] Could not resolve scene path '" << aScenePath
        << "'. Passing authored value through.\n";
    return aScenePath;
}

// -----------------------------------------------------------------------------
// Requested transition path
// -----------------------------------------------------------------------------

void SceneTransitionController::StartRequestedScenePrepare(const Request& aRequest)
{
    if (mySceneTransitionCallback)
    {
        const std::string currentScene = myCurrentSceneCallback ? myCurrentSceneCallback() : std::string{};
        mySceneTransitionCallback(currentScene, aRequest.targetScene);
    }

    myRequestedSceneFuture = std::async(std::launch::async, [scenePath = aRequest.targetScene]()
        {
            // Worker-thread work is limited to cache reads, .leveldata parsing, and
            // plain SceneObjectData creation. Factories/components are built at black.
            return SceneLoadingService::LoadSceneDataAsyncSafe(scenePath);
        });
    myState = State::PreparingRequestedScene;
}

void SceneTransitionController::StartFadeOutWithPreparedData(PreparedSceneData&& somePreparedData)
{
    myPreparedSceneData = std::move(somePreparedData);
    myHasPreparedSceneData = true;
    myState = State::FadingOut;
    myFadeAlpha = 0.0f;
}

void SceneTransitionController::ApplyPreparedScene()
{
    if (!myHasPreparedSceneData)
    {
        FinishFadeIn();
        return;
    }

    myState = State::ApplyingLoadedScene;

    // Main-thread boundary: factories, components, render data, scripts, and
    // gameplay objects are allowed to come alive only after fade-out reaches black.
    LoadedSceneObjects loadedObjects;
    loadedObjects.scenePath = myPreparedSceneData.scenePath;
    loadedObjects.objects = SceneLoadingService::BuildGameObjects(myPreparedSceneData.objects);

    std::vector<SceneObjectData> appliedSceneData = myPreparedSceneData.objects;
    const std::string appliedScenePath = myPreparedSceneData.scenePath;
    const std::string targetSpawnId = myPreparedSceneData.targetSpawnId;

    myHasPreparedSceneData = false;
    myApplySceneCallback(std::move(loadedObjects.objects), appliedScenePath, targetSpawnId);
    StartPreloadFromSceneData(appliedScenePath, appliedSceneData);

    myState = State::FadingIn;
    myFadeAlpha = 1.0f;
}

void SceneTransitionController::FinishFadeIn()
{
    myState = myPreloadFuture.valid() ? State::Preloading : State::Idle;
    myFadeAlpha = 0.0f;
    WorldTransitionService::EndSequence();
    TryConsumeQueuedRequest();
}

// -----------------------------------------------------------------------------
// Queueing
// -----------------------------------------------------------------------------

void SceneTransitionController::QueueRequest(const Request& aRequest)
{
    if (myActiveRequest.targetScene == aRequest.targetScene
        || (myHasQueuedRequest && myQueuedRequest.targetScene == aRequest.targetScene))
    {
        return;
    }

    myQueuedRequest = aRequest;
    myHasQueuedRequest = true;
}

void SceneTransitionController::TryConsumeQueuedRequest()
{
    if (!myHasQueuedRequest)
    {
        return;
    }

    Request queuedRequest = myQueuedRequest;
    myHasQueuedRequest = false;
    RequestTransition(queuedRequest);
}

// -----------------------------------------------------------------------------
// Passive one-slot preload
// -----------------------------------------------------------------------------

void SceneTransitionController::StartPreloadFromSceneData(
    const std::string& aLoadedScenePath,
    const std::vector<SceneObjectData>& someSceneObjects)
{
    if (myPreloadFuture.valid())
    {
        myPreloadFuture.wait();
    }

    myHasPreloadedSceneData = false;
    myPreloadScenePath.clear();

    std::unordered_set<std::string> targets;
    for (const SceneObjectData& objectData : someSceneObjects)
    {
        std::string targetScene;
        if (!objectData.TryGetProperty("targetScene", targetScene))
        {
            continue;
        }

        targetScene = ResolveScenePath(targetScene);
        if (IsValidPreloadTarget(targetScene, aLoadedScenePath))
        {
            targets.insert(targetScene);
        }
    }

    if (targets.size() != 1)
    {
        // Ambiguous authored exits are not guessed. The actual trigger request
        // still loads normally when the player chooses a path.
        myState = myState == State::Preloading ? State::Idle : myState;
        return;
    }

    myPreloadScenePath = *targets.begin();
    myPreloadFuture = std::async(std::launch::async, [scenePath = myPreloadScenePath]()
        {
            // This warms parsed scene data only. It must not construct live
            // GameObjects or GPU/render resources on the worker thread.
            return SceneLoadingService::LoadSceneDataAsyncSafe(scenePath);
        });

    if (myState == State::Idle)
    {
        myState = State::Preloading;
    }
}

void SceneTransitionController::PollPassivePreload()
{
    if (!myPreloadFuture.valid() || !IsFutureReady(myPreloadFuture))
    {
        return;
    }

    LoadedSceneData sceneData = myPreloadFuture.get();
    myPreloadedSceneData.scenePath = sceneData.scenePath;
    myPreloadedSceneData.targetSpawnId.clear();
    myPreloadedSceneData.objects = std::move(sceneData.objects);
    myHasPreloadedSceneData = true;

    if (myState == State::Preloading)
    {
        myState = State::Idle;
    }
}

bool SceneTransitionController::TryUseReadyPreload(const Request& aRequest)
{
    PollPassivePreload();
    if (!myHasPreloadedSceneData || myPreloadedSceneData.scenePath != aRequest.targetScene)
    {
        return false;
    }

    Tga::LoadingProfiler::GetInstance().RecordPhase("SceneTransitionController::UsePreloadedSceneData", 0.0);
    Tga::LoadingProfiler::GetInstance().RecordObjectCount(myPreloadedSceneData.objects.size());
    SceneLoadingService::MarkSceneObjectsReady();

    myPreloadedSceneData.targetSpawnId = aRequest.targetSpawnId;
    StartFadeOutWithPreparedData(std::move(myPreloadedSceneData));
    myHasPreloadedSceneData = false;
    myPreloadScenePath.clear();

    if (mySceneTransitionCallback)
    {
        const std::string currentScene = myCurrentSceneCallback ? myCurrentSceneCallback() : std::string{};
        mySceneTransitionCallback(currentScene, aRequest.targetScene);
    }

    return true;
}

bool SceneTransitionController::TryAttachPendingPreload(const Request& aRequest)
{
    if (!myPreloadFuture.valid() || myPreloadScenePath != aRequest.targetScene)
    {
        return false;
    }

    // The player selected the scene that was already being passively prepared,
    // so the passive future becomes the active requested future.
    myRequestedSceneFuture = std::move(myPreloadFuture);
    myActiveRequest = aRequest;
    myState = State::PreparingRequestedScene;

    if (mySceneTransitionCallback)
    {
        const std::string currentScene = myCurrentSceneCallback ? myCurrentSceneCallback() : std::string{};
        mySceneTransitionCallback(currentScene, aRequest.targetScene);
    }

    return true;
}

bool SceneTransitionController::IsFutureReady(std::future<LoadedSceneData>& aFuture)
{
    return aFuture.valid()
        && aFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}
