#pragma once

#include "SceneLoadingService.h"

#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

class GameObject;

// Owns runtime scene transitions for linear gameplay progression. The controller
// keeps the rules in one place: one active transition, one replacement request,
// and one passive preload for the most likely next scene.
class SceneTransitionController final
{
public:
    // Request shape used by doors, teleporters, scene-manager commands, and
    // future menu/debug callers. targetSpawnId is preserved even though the
    // current apply path does not yet reposition the player from it.
    struct Request
    {
        std::string targetScene;
        std::string targetSpawnId;
        float fadeSeconds = 0.5f;
        // Normal doors leave this false so duplicate current-scene requests are ignored.
        bool forceReload = false;
        std::string source;
    };

    using ApplySceneCallback = std::function<void(
        std::vector<std::unique_ptr<GameObject>>&&,
        const std::string&,
        const std::string&)>;
    using SceneTransitionCallback = std::function<void(const std::string&, const std::string&)>;
    using CurrentSceneCallback = std::function<std::string()>;

    void Initialize(
        ApplySceneCallback anApplySceneCallback,
        SceneTransitionCallback aSceneTransitionCallback,
        CurrentSceneCallback aCurrentSceneCallback);
    void Shutdown();

    // Boot can be synchronous because there is no old gameplay scene to fade out from.
    bool LoadBootScene(const std::string& aScenePath);

    // Starts or queues a transition. If a transition is already applying/fading,
    // only the latest non-duplicate target is kept because progression is linear.
    bool RequestTransition(const Request& aRequest);
    void Update(float aDeltaTime);

    // Waits for outstanding futures before clearing state so stale async work cannot
    // apply into a destroyed gameplay state.
    void CancelPendingWork();

    bool IsTransitionActive() const;
    float GetFadeAlpha() const;

    // Normalizes authored references so Level1, Level1.tgs, and Levels/Level1.tgs
    // match the same cache/preload key.
    static std::string ResolveScenePath(const std::string& aScenePath);

private:
    enum class State
    {
        Idle,
        // Passive next-scene data preparation. This never builds GameObjects.
        Preloading,
        // Requested transition data is being parsed/read on a worker thread.
        PreparingRequestedScene,
        FadingOut,
        // Full-black section where main-thread object construction/apply happens.
        ApplyingLoadedScene,
        FadingIn
    };

    struct PreparedSceneData
    {
        std::string scenePath;
        std::string targetSpawnId;
        std::vector<SceneObjectData> objects;
    };

    void StartRequestedScenePrepare(const Request& aRequest);
    void StartFadeOutWithPreparedData(PreparedSceneData&& somePreparedData);
    void ApplyPreparedScene();
    void FinishFadeIn();
    void QueueRequest(const Request& aRequest);
    void TryConsumeQueuedRequest();

    // Uses authored targetScene properties from the scene that just loaded. If more
    // than one unique exit is found, it avoids guessing and skips preload.
    void StartPreloadFromSceneData(
        const std::string& aLoadedScenePath,
        const std::vector<SceneObjectData>& someSceneObjects);
    void PollPassivePreload();
    bool TryUseReadyPreload(const Request& aRequest);
    bool TryAttachPendingPreload(const Request& aRequest);

    static bool IsFutureReady(std::future<LoadedSceneData>& aFuture);

    ApplySceneCallback myApplySceneCallback;
    SceneTransitionCallback mySceneTransitionCallback;
    CurrentSceneCallback myCurrentSceneCallback;

    State myState = State::Idle;
    Request myActiveRequest;
    Request myQueuedRequest;
    bool myHasQueuedRequest = false;

    std::future<LoadedSceneData> myRequestedSceneFuture;
    PreparedSceneData myPreparedSceneData;
    bool myHasPreparedSceneData = false;

    std::future<LoadedSceneData> myPreloadFuture;
    PreparedSceneData myPreloadedSceneData;
    bool myHasPreloadedSceneData = false;
    std::string myPreloadScenePath;

    float myFadeAlpha = 0.0f;
    float myFadeSeconds = 0.5f;
};
