#pragma once
#include "State.hpp"
#include "CombatSystem.h"
#include "RuntimeCollisionSystem.h"
#include "WorldTransitionService.h"

#include <string>
#include <vector>

class InGame : public State, public WorldTransitionService::Listener
{
public:
	InGame() = default;
	~InGame() override;

	void Init(CameraSystem& aCamera, const char* argv[]) override;
	eState Update() override;
	void Render() override;
	bool RequestSceneTransition(
		const std::string& aTargetScene,
		const std::string& aTargetSpawnId,
		float aFadeOutSeconds) override;

private:
	enum class SceneFadeState
	{
		None,
		FadingOutForSceneLoad,
		FadingInAfterSceneLoad
	};

	void ConsumeCollisionContacts(const std::vector<CollisionContact>& someContacts);
	void UpdateSceneFade(float aDeltaTime);
	void RenderSceneFadeOverlay();

	CombatSystem myCombatSystem;
	RuntimeCollisionSystem myRuntimeCollisionSystem;
	SceneFadeState mySceneFadeState = SceneFadeState::None;
	std::string myPendingSceneTransitionTarget;
	std::string myPendingSceneTransitionSpawnId;
	float mySceneFadeAlpha = 0.0f;
	float mySceneFadeDuration = 0.5f;
};
