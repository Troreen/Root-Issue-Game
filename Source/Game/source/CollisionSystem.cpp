//#include "CollisionSystemComponent.h"
//
//#include "GameObject.h"
//#include "CollisionHandler.h"
//#include "GameObjectTags.h"
//#include "PlayerController.h"
//#include "PlayerAnimator.h"
//#include "SchnozController.h"
//#include "BreakableTileController.h"
//#include "SchnozAnimator.h"
//#include "DamageableComponent.h"
//#include "CheckpointComponent.h"
//#include "LevelEndComponent.h"
//#include "BoxColliderComponent.h"
//#include "SphereColliderComponent.h"
//#include "SpeechBubbleComponent.h"
//#include "BloodSeaComponent.h"
//#include "SceneManager.h"
//#include "HoverSystem.h"
//#include "VfxSystem.h"
//#include "CameraSystem.h"
//#include "Menu/StateStack.h"
//
//#include <CommonUtilities/InputHandler.h>
//
//#include <nlohmann/json.hpp>
//
//#include <filesystem>
//#include <fstream>
//#include <iostream>
//
//namespace
//{
//	constexpr float kDefaultWorldDeathYThreshold = -2000.0f;
//	constexpr float kCollectibleRefreshIntervalSeconds = 0.25f;
//	constexpr const char* kCoinProgressRelativePath = "Source/Game/data/settings/level_coin_progress.json";
//
//	std::filesystem::path GetCoinProgressPath()
//	{
//		const std::filesystem::path cwd = std::filesystem::current_path();
//		const std::filesystem::path fromCwd = cwd / kCoinProgressRelativePath;
//		if (std::filesystem::exists(fromCwd.parent_path()))
//		{
//			return fromCwd;
//		}
//
//		const std::filesystem::path fromParent = cwd.parent_path() / kCoinProgressRelativePath;
//		if (std::filesystem::exists(fromParent.parent_path()))
//		{
//			return fromParent;
//		}
//
//		return fromParent;
//	}
//
//	bool IsEnemyTag(const std::string& aTag)
//	{
//		return aTag == GameObjectTags::Schnoz_II || aTag == GameObjectTags::Schnoz_III;
//	}
//
//	bool IsBreakableTag(const std::string& aTag)
//	{
//		return aTag == GameObjectTags::BreakableBlock;
//	}
//
//	bool IsPickupTag(const std::string& aTag)
//	{
//		return aTag == GameObjectTags::Coin || aTag == GameObjectTags::Key || aTag == GameObjectTags::PowerUp;
//	}
//
//	bool IsRespawnTrackedTag(const std::string& aTag)
//	{
//		return IsEnemyTag(aTag) || IsBreakableTag(aTag) || IsPickupTag(aTag);
//	}
//
//	bool ShouldResetForCheckpoint(const std::string& aTag)
//	{
//		return IsEnemyTag(aTag) || IsBreakableTag(aTag);
//	}
//}
//
//void CollisionSystemComponent::SetGameObjects(std::vector<std::unique_ptr<GameObject>>* aObjects)
//{
//	myGameObjects = aObjects;
//}
//
//void CollisionSystemComponent::SetPlayer(GameObject* aPlayer)
//{
//	myPlayer = aPlayer;
//}
//
//void CollisionSystemComponent::SetHoverSystem(HoverSystem* aHoverSystem)
//{
//	myHoverSystem = aHoverSystem;
//}
//
//void CollisionSystemComponent::SetCameraSystem(CameraSystem* aCameraSystem)
//{
//	myCameraSystem = aCameraSystem;
//}
//
//void CollisionSystemComponent::SetInputHandler(const CommonUtilities::InputHandler* anInputHandler)
//{
//	myInputHandler = anInputHandler;
//}
//
//void CollisionSystemComponent::ResetAllPickupCounters()
//{
//	CollisionHandler::ResetCollectedCoins();
//	CollisionHandler::ResetCollectedKeys();
//	myRankSystem = nullptr;
//}
//
//void CollisionSystemComponent::CaptureInitialRespawnState()
//{
//	myRespawnSnapshots.clear();
//	myHasCapturedRespawnSnapshots = false;
//
//	if (myGameObjects == nullptr)
//	{
//		return;
//	}
//
//	for (auto& object : *myGameObjects)
//	{
//		if (!object || object.get() == myPlayer)
//		{
//			continue;
//		}
//
//		const std::string& tag = object->GetTag();
//		if (!IsRespawnTrackedTag(tag))
//		{
//			continue;
//		}
//
//		RuntimeObjectSnapshot snapshot;
//		snapshot.tag = tag;
//		snapshot.position = object->GetTransform().GetPosition();
//		snapshot.rotation = object->GetTransform().GetRotation();
//		snapshot.scale = object->GetTransform().GetScale();
//		snapshot.hitbox = object->GetHitbox();
//		snapshot.wasActive = object->IsActive();
//
//		if (IsEnemyTag(tag))
//		{
//			if (auto* schnoz = object->GetComponent<SchnozController>())
//			{
//				snapshot.hasSchnozState = true;
//				snapshot.schnoz.speed = schnoz->GetSpeed();
//				snapshot.schnoz.defencePoints = schnoz->GetDefencePoints();
//				snapshot.schnoz.facingRight = schnoz->IsFacingRight();
//			}
//		}
//
//		myRespawnSnapshots[object.get()] = snapshot;
//	}
//
//	myHasCapturedRespawnSnapshots = !myRespawnSnapshots.empty();
//}
//
//void CollisionSystemComponent::ApplyRespawnResetPolicy(bool aIsCheckpointRespawn)
//{
//	if (myGameObjects == nullptr || !myHasCapturedRespawnSnapshots)
//	{
//		return;
//	}
//
//	for (auto& object : *myGameObjects)
//	{
//		if (!object || object.get() == myPlayer)
//		{
//			continue;
//		}
//
//		auto snapshotIt = myRespawnSnapshots.find(object.get());
//		if (snapshotIt == myRespawnSnapshots.end())
//		{
//			continue;
//		}
//
//		const RuntimeObjectSnapshot& snapshot = snapshotIt->second;
//		if (aIsCheckpointRespawn && !ShouldResetForCheckpoint(snapshot.tag))
//		{
//			continue;
//		}
//
//		auto& transform = object->GetTransform();
//		transform.SetPosition(snapshot.position);
//		transform.SetRotation(snapshot.rotation);
//		transform.SetScale(snapshot.scale);
//		object->SetHitbox(snapshot.hitbox);
//		object->SetActive(snapshot.wasActive);
//
//		if (IsEnemyTag(snapshot.tag))
//		{
//			if (auto* schnoz = object->GetComponent<SchnozController>())
//			{
//				if (snapshot.hasSchnozState)
//				{
//					schnoz->ResetForRespawn(
//						snapshot.schnoz.speed,
//						snapshot.schnoz.defencePoints,
//						snapshot.schnoz.facingRight);
//				}
//				else
//				{
//					schnoz->ResetForRespawn();
//				}
//			}
//
//			if (auto* schnozAnimator = object->GetComponent<SchnozAnimator>())
//			{
//				schnozAnimator->ResetForRespawn();
//			}
//		}
//		else if (IsBreakableTag(snapshot.tag))
//		{
//			if (auto* breakable = object->GetComponent<BreakableTileController>())
//			{
//				breakable->ResetForRespawn();
//			}
//		}
//	}
//
//	if (!aIsCheckpointRespawn)
//	{
//		if (myRankSystem != nullptr)
//		{
//			myRankSystem->ResetForLevel();
//		}
//		ResetAllPickupCounters();
//		myCollectibleManager.RefreshFromWorld(*myGameObjects);
//		myCollectibleRefreshTimer = 0.0f;
//		myLastObservedCoinCount = 0;
//	}
//}
//
//void CollisionSystemComponent::ResetLevelRuntimeState()
//{
//	EnsureCoinProgressLoaded();
//
//	myHaveAlreadyTriggerBloodSea = false;
//	myHasRespawnPoint = false;
//	myLastCheckpointRespawnPosition = { 0.0f, 0.0f, 0.0f };
//	myRespawnFacingRight = true;
//	myCollectibleManager.Clear();
//	myCollectibleRefreshTimer = 0.0f;
//	if (myRankSystem != nullptr)
//	{
//		myRankSystem->ResetForLevel();
//	}
//	ResetAllPickupCounters();
//	myLastObservedCoinCount = 0;
//	CaptureInitialRespawnState();
//
//}
//
//int CollisionSystemComponent::GetSavedCoinsForLevel(const std::string& aScenePath)
//{
//	EnsureCoinProgressLoaded();
//
//	auto it = mySavedCoinsByLevel.find(aScenePath);
//	if (it == mySavedCoinsByLevel.end())
//	{
//		return 0;
//	}
//
//	return it->second;
//}
//
//void CollisionSystemComponent::EnsureCoinProgressLoaded()
//{
//	if (myHasLoadedCoinProgress)
//	{
//		return;
//	}
//
//	myHasLoadedCoinProgress = true;
//	mySavedCoinsByLevel.clear();
//
//	const std::filesystem::path filePath = GetCoinProgressPath();
//	if (!std::filesystem::exists(filePath))
//	{
//		return;
//	}
//
//	std::ifstream inFile(filePath);
//	if (!inFile.is_open())
//	{
//		return;
//	}
//
//	nlohmann::json root;
//	try
//	{
//		inFile >> root;
//	}
//	catch (...)
//	{
//		return;
//	}
//
//	if (!root.contains("PointsPerLevel") || !root["PointsPerLevel"].is_object())
//	{
//		return;
//	}
//
//	if (!root.contains("KeysPerLevel") || !root["KeysPerLevel"].is_object())
//	{
//		return;
//	}
//
//	for (const auto& [scenePath, coinValue] : root["PointsPerLevel"].items())
//	{
//		if (!coinValue.is_number_integer())
//		{
//			continue;
//		}
//
//		const int savedCoins = coinValue.get<int>();
//		if (savedCoins < 0)
//		{
//			continue;
//		}
//
//		mySavedCoinsByLevel[scenePath] = savedCoins;
//	}
//
//	for (const auto& [scenePath, KeysValue] : root["KeysPerLevel"].items())
//	{
//		if (!KeysValue.is_number_integer())
//		{
//			continue;
//		}
//
//		const int savedKeys = KeysValue.get<int>();
//		if (savedKeys < 0)
//		{
//			continue;
//		}
//
//		mySavedKeysByLevel[scenePath] = savedKeys;
//	}
//}
//
//void CollisionSystemComponent::SaveCoinProgress() const
//{
//	nlohmann::json root;
//	root["PointsPerLevel"] = nlohmann::json::object();
//	root["KeysPerLevel"] = nlohmann::json::object();
//
//	for (const auto& [scenePath, coinCount] : mySavedCoinsByLevel)
//	{
//		root["PointsPerLevel"][scenePath] = coinCount;
//	}
//
//	for (const auto& [scenePath, KeysValue] : mySavedKeysByLevel)
//	{
//		root["KeysPerLevel"][scenePath] = KeysValue;
//	}
//
//	const std::filesystem::path filePath = GetCoinProgressPath();
//	std::filesystem::create_directories(filePath.parent_path());
//
//	std::ofstream outFile(filePath, std::ios::trunc);
//	if (!outFile.is_open())
//	{
//		return;
//	}
//
//	outFile << root.dump(2);
//}
//
//void CollisionSystemComponent::RecordCurrentLevelCoinProgress()
//{
//	EnsureCoinProgressLoaded();
//
//	const std::string& currentScene = SceneManager::GetInstance().GetCurrentScene();
//	if (currentScene.empty())
//	{
//		return;
//	}
//
//	const int currentCoins = std::stoi(myRankSystem->GetScorePoints().GetText());
//	const int currentKeys = CollisionHandler::GetCollectedKeys();
//	if (currentCoins < 0)
//	{
//		return;
//	}
//
//	int& savedCoins = mySavedCoinsByLevel[currentScene];
//	if (currentCoins > savedCoins)
//	{
//		savedCoins = currentCoins;
//		SaveCoinProgress();
//	}
//
//	int& savedCKeys = mySavedKeysByLevel[currentScene];
//	if (currentKeys > savedCKeys)
//	{
//		savedCKeys = currentKeys;
//		SaveCoinProgress();
//	}
//
//	myLastObservedCoinCount = currentCoins;
//}
//
//void CollisionSystemComponent::OnUpdate(float aDeltaTime)
//{
//	if (!myPlayer || !myGameObjects)
//	{
//		return;
//	}
//
//	EnsureCoinProgressLoaded();
//
//	auto* playerController = myPlayer->GetComponent<PlayerController>();
//	if (!playerController)
//	{
//		return;
//	}
//
//	auto* playerDamageable = myPlayer->GetComponent<DamageableComponent>();
//
//	std::vector<GameObject*> collidables;
//	std::vector<GameObject*> enemiesCollision;
//	std::vector<GameObject*> triggerCollision;
//	std::vector<GameObject*> deathBoxesCollision;
//	GameObject* bossCollision = nullptr;
//	GameObject* bloodSeaObject = nullptr;
//	BloodSeaComponent* bloodSeaData = nullptr;
//	float worldDeathYThreshold = kDefaultWorldDeathYThreshold;
//	collidables.reserve(myGameObjects->size());
//
//	for (auto& obj : *myGameObjects)
//	{
//		if (!obj || !obj->IsActive() || obj.get() == myPlayer)
//		{
//			continue;
//		}
//
//		const std::string& tag = obj->GetTag();
//		if (tag == GameObjectTags::Ground ||
//			tag == GameObjectTags::Platform ||
//			tag == GameObjectTags::BreakableBlock)
//		{
//			collidables.push_back(obj.get());
//		}
//
//		if (tag == GameObjectTags::Schnoz_II || tag == GameObjectTags::Schnoz_III)
//		{
//			enemiesCollision.push_back(obj.get());
//		}
//
//		if (tag == GameObjectTags::Boss)
//		{
//			bossCollision = obj.get();
//		}
//
//		if (tag == GameObjectTags::BloodSea)
//		{
//			bloodSeaObject = obj.get();
//			bloodSeaData = obj->GetComponent<BloodSeaComponent>();
//		}
//
//		if (tag == GameObjectTags::AutocamTrigger ||
//			tag == GameObjectTags::Trigger ||
//			tag == GameObjectTags::Checkpoint ||
//			tag == GameObjectTags::LevelEnd ||
//			tag == GameObjectTags::LockCamTrigger ||
//			tag == GameObjectTags::Dialogue)
//		{
//			triggerCollision.push_back(obj.get());
//		}
//
//		if (tag == GameObjectTags::RankingSystem)
//		{
//			if (myRankSystem == nullptr)
//			{
//				myRankSystem = obj->GetComponent<RankSystem>();
//				StateStack::GetInstance()->SetRankingComponent(myRankSystem);
//			}
//		}
//
//		if (tag == GameObjectTags::WorldDeathThreshold)
//		{
//			worldDeathYThreshold = obj->GetTransform().GetPosition().y;
//		}
//
//		if (tag == GameObjectTags::DeathBox)
//		{
//			deathBoxesCollision.push_back(obj.get());
//		}
//	}
//
//	if (playerDamageable && !playerDamageable->IsDead())
//	{
//		const float playerY = myPlayer->GetTransform().GetPosition().y;
//		if (playerY < worldDeathYThreshold)
//		{
//			playerDamageable->SetCurrentHealth(0);
//			std::cout << "[Death] Player fell below world threshold at y=" << playerY << "\n";
//		}
//	}
//
//	const bool isBloodSeaHazardActive =
//		bloodSeaObject && bloodSeaObject->IsActive() && bloodSeaData && bloodSeaData->IsHazardActive();
//	if (!isBloodSeaHazardActive)
//	{
//		if (myHaveAlreadyTriggerBloodSea)
//		{
//			AudioManager::GetInstance().Stop(EAudioSource::Blood_Bubbling_SFX);
//		}
//		myHaveAlreadyTriggerBloodSea = false;
//	}
//
//	if (myRankSystem == nullptr)
//	{
//		myRankSystem = new RankSystem();
//		StateStack::GetInstance()->SetRankingComponent(myRankSystem);
//	}
//
//	CollisionHandler::PlayerAgainstDeathBoxes(*playerController, deathBoxesCollision);
//
//	CollisionHandler::PlayerAgainstTriggerBox(*playerController, triggerCollision);
//
//	//const bool isPressingEnterLevelEnd = myInputHandler && myInputHandler->IsKeyPressed(Keys::E);
//	const bool isPressingEnterLevelEnd = playerController->CheckWorldInteract();
//
//	for (GameObject* triggerObject : triggerCollision)
//	{
//		const std::string& tag = triggerObject->GetTag();
//
//		if (tag == GameObjectTags::Dialogue)
//		{
//			auto* triggerBox = triggerObject->GetComponent<BoxColliderComponent>();
//			auto* dialogue = triggerObject->GetComponent<SpeechBubbleComponent>();
//
//			if (dialogue->GetTriggerInteract() == false)
//			{
//				continue;
//			}
//
//			if (triggerBox->IsInside() && dialogue->GetActive() == false && triggerBox->IsEnabled() == true)
//			{
//				dialogue->SetActive(true);
//				dialogue->ResetTime();
//				dialogue->SetTarget(myPlayer);
//				dialogue->SetFollow(true);
//				triggerBox->SetEnabled(false);
//			}
//
//			continue;
//		}
//
//		if (tag == GameObjectTags::Checkpoint)
//		{
//			auto* triggerBox = triggerObject->GetComponent<BoxColliderComponent>();
//			auto* checkpoint = triggerObject->GetComponent<CheckpointComponent>();
//			if (!triggerBox || !checkpoint)
//			{
//				continue;
//			}
//
//			if (triggerBox->IsInside() && checkpoint->Activate())
//			{
//				myLastCheckpointRespawnPosition = checkpoint->GetRespawnPosition();
//				myHasRespawnPoint = true;
//				myRespawnFacingRight = checkpoint->ShouldRespawnFacingRight();
//
//				AudioManager::GetInstance().Play(EAudioSource::Checkpoint);
//			}
//			continue;
//		}
//
//		if (tag == GameObjectTags::LevelEnd)
//		{
//			auto* triggerBox = triggerObject->GetComponent<BoxColliderComponent>();
//			auto* levelEnd = triggerObject->GetComponent<LevelEndComponent>();
//			if (!triggerBox || !levelEnd)
//			{
//				continue;
//			}
//
//			if (triggerBox->IsInside() && isPressingEnterLevelEnd)
//			{
//				std::cout << "[LevelEnd] Enter attempt on '" << triggerObject->GetName() << "'\n";
//				if (levelEnd->BeginLevelExit())
//				{
//					playerController->SetFacingRight(true);
//					RecordCurrentLevelCoinProgress();
//
//					std::cout << "[LevelEnd] Enter accepted, starting level transition sequence\n";
//					AudioManager::GetInstance().PlayLevelMusic("");
//					AudioManager::GetInstance().Stop(EAudioSource::Blood_Bubbling_SFX);
//					AudioManager::GetInstance().Play(EAudioSource::Level_Cleared);
//					AudioManager::GetInstance().Play(EAudioSource::Exit_Door_SFX);
//					myPlayer->GetComponent<PlayerAnimator>()->StartWin();
//				}
//				else
//				{
//					std::cout << "[LevelEnd] Enter rejected (already transitioning, missing target level, or not inside trigger)\n";
//				}
//			}
//		}
//
//	}
//
//	if (isBloodSeaHazardActive && playerDamageable && !playerDamageable->IsDead())
//	{
//		if (!myHaveAlreadyTriggerBloodSea)
//		{
//			AudioManager::GetInstance().Play(EAudioSource::Blood_Bubbling_SFX);
//			myHaveAlreadyTriggerBloodSea = true;
//		}
//		const float playerY = myPlayer->GetTransform().GetPosition().y;
//		const float bloodSeaY = bloodSeaObject->GetTransform().GetPosition().y;
//		const float relativeY = playerY - bloodSeaY;
//		const float bloodSeaKillDistanceY = bloodSeaData->GetKillDistanceY();
//
//		if (relativeY <= bloodSeaKillDistanceY)
//		{
//			playerDamageable->SetCurrentHealth(0);
//			std::cout << "[Death] BloodSea too close in Y. playerY=" << playerY
//				<< " bloodSeaY=" << bloodSeaY
//				<< " relativeY=" << relativeY << "\n";
//		}
//	}
//
//	myRankSystem->GetScorePoints();
//	myCollectibleRefreshTimer += aDeltaTime;
//	if (myCollectibleRefreshTimer >= kCollectibleRefreshIntervalSeconds || myCollectibleManager.GetItems().empty())
//	{
//		myCollectibleManager.RefreshFromWorld(*myGameObjects);
//		myCollectibleRefreshTimer = 0.0f;
//	}
//
//	if (!myCollectibleManager.GetItems().empty())
//	{
//		CollisionHandler::PlayerAgainstPickups(*playerController, myCollectibleManager, myRankSystem);
//	}
//
//	const int currentCoins = CollisionHandler::GetCollectedCoins();
//	if (currentCoins != myLastObservedCoinCount)
//	{
//		RecordCurrentLevelCoinProgress();
//	}
//
//	if (!collidables.empty())
//	{
//		AudioManager::GetInstance().PlayPlayerSFX(playerController->GetOwner()->GetComponent<PlayerAnimator>()->GetCurrentAnimationIndex());
//		CollisionHandler::PlayerAgainstMany(*playerController, collidables);
//
//		GameObject* hoverLandingObject = nullptr;
//		float hoverLandingImpactForce = 0.0f;
//		bool hoverLandingWasGroundPound = false;
//		if (myHoverSystem && playerController->ConsumeHoverLanding(hoverLandingObject, hoverLandingImpactForce, hoverLandingWasGroundPound))
//		{
//			myHoverSystem->ApplyLandingDipForObject(hoverLandingObject, hoverLandingImpactForce, hoverLandingWasGroundPound);
//			VfxService::SpawnWorldEffect("platform_dust_fall", hoverLandingObject->GetTransform().GetPosition(), 1.0f, 1.0f);
//		}
//
//		float shortestDistenceXToSchnozII = 99999;
//		float shortestDistenceXToSchnozIII = 99999;
//
//		float shortestDistenceYToSchnozII = 99999;
//		float shortestDistenceYToSchnozIII = 99999;
//
//		for (int schnozIndex = static_cast<int>(enemiesCollision.size() - 1); schnozIndex > -1; schnozIndex--)
//		{
//			float deltaDistanceX = enemiesCollision[schnozIndex]->GetTransform().GetPosition().x - playerController->GetOwner()->GetTransform().GetPosition().x;
//			float deltaDistanceY = enemiesCollision[schnozIndex]->GetTransform().GetPosition().Distance(playerController->GetOwner()->GetTransform().GetPosition());
//			if (enemiesCollision[schnozIndex]->GetTag() == GameObjectTags::Schnoz_II)
//			{
//				if (deltaDistanceY < shortestDistenceYToSchnozII)
//				{
//					shortestDistenceXToSchnozII = deltaDistanceX;
//					shortestDistenceYToSchnozII = deltaDistanceY;
//				}
//			}
//			else
//			{
//				if (deltaDistanceY < shortestDistenceYToSchnozIII)
//				{
//					shortestDistenceXToSchnozIII = deltaDistanceX;
//					shortestDistenceYToSchnozIII = deltaDistanceY;
//				}
//			}
//
//			CollisionHandler::SchnozAgainstMany(*enemiesCollision[schnozIndex], collidables);
//			CollisionHandler::SchnozCheckIfAtEdge(*enemiesCollision[schnozIndex], collidables);
//
//			if (CollisionHandler::PlayerAgainstSchnoz(*playerController, *enemiesCollision[schnozIndex]))
//			{
//				if (enemiesCollision[schnozIndex]->GetComponent<SchnozController>()->IsPrimedForDestruction())
//				{
//					std::cout << "[Combat] Enemy killed: '" << enemiesCollision[schnozIndex]->GetName() << "'\n";
//
//					if (enemiesCollision[schnozIndex]->GetTag() == GameObjectTags::Schnoz_II)
//					{
//						AudioManager::GetInstance().Play(EAudioSource::Schnoz_II_Death);
//					}
//					else
//					{
//						AudioManager::GetInstance().Play(EAudioSource::Schnoz_III_Death);
//					}
//				}
//				else
//				{
//					AudioManager::GetInstance().Play(EAudioSource::Player_Takes_Damage);
//					// Camera shake disabled
//					//if (myCameraSystem)
//					//{
//					//	myCameraSystem->TriggerCameraShake(0.18f, 35.0f);
//					//}
//				}
//			}
//			if (enemiesCollision[schnozIndex]->GetComponent<SchnozController>()->IsPrimedForDestruction())
//			{
//				if (!enemiesCollision[schnozIndex]->IsActive())
//				{
//					enemiesCollision.erase(enemiesCollision.begin() + schnozIndex);
//					continue;
//				}
//			}
//		}
//
//		shortestDistenceXToSchnozII = 0.0f + (shortestDistenceXToSchnozII * 0.000625f);
//		shortestDistenceXToSchnozIII = 0.0f + (shortestDistenceXToSchnozIII * 0.000625f);
//		shortestDistenceYToSchnozII = 0.0f + (shortestDistenceYToSchnozII * 0.000625f);
//		shortestDistenceYToSchnozIII = 0.0f + (shortestDistenceYToSchnozIII * 0.000625f);
//
//		if (shortestDistenceXToSchnozII > 1)
//		{
//			shortestDistenceXToSchnozII = 1;
//		}
//		else if (shortestDistenceXToSchnozII < -1)
//		{
//			shortestDistenceXToSchnozII = -1;
//		}
//
//		if (shortestDistenceXToSchnozIII > 1)
//		{
//			shortestDistenceXToSchnozIII = 1;
//		}
//		else if (shortestDistenceXToSchnozIII < -1)
//		{
//			shortestDistenceXToSchnozIII = -1;
//		}
//
//		if (shortestDistenceYToSchnozII > 1)
//		{
//			shortestDistenceYToSchnozII = 1;
//		}
//		else if (shortestDistenceYToSchnozII < -1)
//		{
//			shortestDistenceYToSchnozII = -1;
//		}
//
//		if (shortestDistenceYToSchnozIII > 1)
//		{
//			shortestDistenceYToSchnozIII = 1;
//		}
//		else if (shortestDistenceYToSchnozIII < -1)
//		{
//			shortestDistenceYToSchnozIII = -1;
//		}
//
//		AudioManager::GetInstance().SetPosition(EAudioSource::Schnoz_II_Walk, CommonUtilities::Vector2<float>(shortestDistenceXToSchnozII, 1.0f));
//		AudioManager::GetInstance().SetPosition(EAudioSource::Schnoz_III_Walk, CommonUtilities::Vector2<float>(shortestDistenceXToSchnozIII, 1.0f));
//		AudioManager::GetInstance().SetVolumeOn(EAudioSource::Schnoz_II_Walk, 1.0f - std::abs(shortestDistenceYToSchnozII));
//		AudioManager::GetInstance().SetVolumeOn(EAudioSource::Schnoz_III_Walk, 1.0f - std::abs(shortestDistenceYToSchnozIII));
//
//		//AudioManager::GetInstance().Play(EAudioSource::Schnoz_II_Walk); // Kanske ändra positionen av detta till i walk state?
//		//AudioManager::GetInstance().Play(EAudioSource::Schnoz_III_Walk);
//
//
//		for (int BreakableTileIndex = static_cast<int>(collidables.size() - 1); BreakableTileIndex > -1; BreakableTileIndex--)
//		{
//			if (collidables[BreakableTileIndex]->GetTag() == GameObjectTags::BreakableBlock)
//			{
//				if (collidables[BreakableTileIndex]->GetComponent<BreakableTileController>()->GetAlreadyBrokenBool())
//				{
//					AudioManager::GetInstance().Stop(EAudioSource::Breakable_Tile_SFX);
//					AudioManager::GetInstance().Play(EAudioSource::Breakable_Tile_SFX);
//					collidables[BreakableTileIndex]->SetActive(false);
//					collidables.erase(collidables.begin() + BreakableTileIndex);
//				}
//			}
//		}
//	}
//
//	CollisionHandler::SchnozAgainstSchnoz(enemiesCollision, enemiesCollision);
//
//	if (bossCollision)
//	{
//		AudioManager::GetInstance().PlayBossSFX(bossCollision->GetComponent<BossAnimator>()->GetCurrentAnimationIndex());
//		CollisionHandler::PlayerAgainstBoss(*playerController, *bossCollision);
//	}
//
//	if (playerDamageable)
//	{
//		if (playerDamageable->IsDead())
//		{
//			const bool isCheckpointRespawn = myHasRespawnPoint;
//			const Vector3f respawnPosition = isCheckpointRespawn
//				? myLastCheckpointRespawnPosition
//				: Vector3f::Zero;
//			const bool shouldFaceRight = isCheckpointRespawn ? myRespawnFacingRight : true;
//
//			playerController->BlockMovement();
//
//			for (auto& object : *myGameObjects)
//			{
//				if (!object)
//				{
//					continue;
//				}
//
//				if (auto* sphere = object->GetComponent<SphereColliderComponent>())
//				{
//					if (sphere->IsTrigger())
//					{
//						sphere->OnTriggerExit();
//					}
//				}
//
//				if (auto* box = object->GetComponent<BoxColliderComponent>())
//				{
//					if (box->IsTrigger())
//					{
//						box->OnTriggerExit();
//					}
//				}
//			}
//
//			if (playerController->GetOwner()->GetComponent<PlayerAnimator>()->HasPlayerDied())
//			{
//				ApplyRespawnResetPolicy(isCheckpointRespawn);
//				playerController->RespawnAt(respawnPosition, shouldFaceRight);
//				myHaveAlreadyTriggerBloodSea = false;
//
//				playerDamageable->SetCurrentHealth(playerDamageable->GetMaxHealth());
//
//				playerController->UnblockMovement();
//			}
//		}
//	}
//}