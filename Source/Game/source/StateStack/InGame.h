#pragma once
#include "State.hpp"
#include "CombatSystem.h"
#include "RuntimeCollisionSystem.h"
#include "SceneTransitionController.h"
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
	void ApplyTransitionScene(
		std::vector<std::unique_ptr<GameObject>>&& someObjects,
		const std::string& aScenePath,
		const std::string& aTargetSpawnId);
	void ConsumeCollisionContacts(const std::vector<CollisionContact>& someContacts);
	void RenderSceneFadeOverlay();

	CombatSystem myCombatSystem;
	RuntimeCollisionSystem myRuntimeCollisionSystem;
	SceneTransitionController mySceneTransitionController;
};
