#pragma once

#include "SceneLoadingService.h"

#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

class GameObject;

class SceneTransitionController final
{
public:
    struct Request
    {
        std::string targetScene;
        std::string targetSpawnId;
        float fadeSeconds = 0.5f;
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

    bool LoadBootScene(const std::string& aScenePath);
    bool RequestTransition(const Request& aRequest);
    void Update(float aDeltaTime);
    void CancelPendingWork();

    bool IsTransitionActive() const;
    float GetFadeAlpha() const;

    static std::string ResolveScenePath(const std::string& aScenePath);

private:
    enum class State
    {
        Idle,
        Preloading,
        PreparingRequestedScene,
        FadingOut,
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
