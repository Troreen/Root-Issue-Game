//#include "CollisionHandler.h"
//
//#include <CommonUtilities/Intersection.hpp>
//#include <cmath>
//#include <iostream>
//
//#include "GameObject.h"
//#include "PlayerController.h"
//#include "SchnozController.h"
//#include "Boss/BossAnimator.h"
//#include "BreakableTileController.h"
//#include "BoxColliderComponent.h"
//#include "SphereColliderComponent.h"
//#include "HoverComponent.h"
//#include "Collectibles.h"
//#include "DamageableComponent.h"
//#include "EyeTrigger.h"
//#include "GameObjectTags.h"
//#include "CollectibleManager.h"
//#include "AudioManager.h"
//#include "RankSystem.h"
//#include "VfxSystem.h"
//
//using namespace CommonUtilities;
//
//namespace
//{
//	int gCurrentLife = 2;
//	int gCollectedCoins = 0;
//	int gCollectedKeys = 0;
//}
//
//// ---------------------------------------------------------------------------
//// Helper: get the AABB for any game object.
//// Prefers BoxColliderComponent bounds if present.
//// ---------------------------------------------------------------------------
//static AABB3D<float> GetEffectiveAABB(const GameObject& aObj)
//{
//	auto* box = aObj.GetComponent<BoxColliderComponent>();
//	if (box)
//	{
//		return box->GetAABB();
//	}
//
//	return CommonUtilities::AABB3D<float>({ 0, 0, 0 }, { 0, 0, 0 });
//}
//
//// ---------------------------------------------------------------------------
//bool CollisionHandler::PlayerAgainstMany(PlayerController& inoutPlayer, std::vector<GameObject*>& inoutObjectList)
//{
//	bool anyCollision = false;
//	bool bottomCollision = false;
//	bool leftCollision = false;
//	bool rightCollision = false;
//
//	const AABB3D<float> playerAABB = GetEffectiveAABB(*inoutPlayer.GetOwner());
//	const float playerForwardSign = inoutPlayer.IsFacingRight() ? 1.0f : -1.0f;
//	const Vector3<float>& playerMin = playerAABB.GetMin();
//	const Vector3<float>& playerMax = playerAABB.GetMax();
//	const Vector3<float> playerCenter = (playerMin + playerMax) * 0.5f;
//	AABB3D<float> playerHeadCollsion = AABB3D<float>(
//		Vector3<float>(playerAABB.GetMin().x, playerAABB.GetMax().y, playerAABB.GetMin().z),
//		Vector3<float>(playerAABB.GetMax().x, playerAABB.GetMax().y + 50.0f, playerAABB.GetMax().z)
//		);
//
//	std::vector<GameObject*> checkThese = {};
//	std::vector<GameObject*> Breakable = {};
//
//	{
//		const AABB3D<float> wideAreaBox = {
//			Vector3f(playerMin.x - 500, playerMin.y - 500, playerMin.z - 500),
//			Vector3f(playerMax.x + 500, playerMax.y + 500, playerMax.z + 500)
//		};
//		for (size_t i = 0; i < inoutObjectList.size(); i++)
//		{
//			if (IntersectionAABBAABB(wideAreaBox, GetEffectiveAABB(*inoutObjectList[i])))
//			{
//				checkThese.push_back(inoutObjectList[i]);
//				if (inoutObjectList[i]->GetTag() == GameObjectTags::BreakableBlock)
//				{
//					Breakable.push_back(inoutObjectList[i]);
//				}
//			}
//		}
//	}
//	for (size_t i = 0; i < Breakable.size(); i++)
//	{
//		if (IntersectionAABBAABB(playerAABB, GetEffectiveAABB(*Breakable[i])) || IntersectionAABBAABB(playerHeadCollsion, GetEffectiveAABB(*Breakable[i])))
//		{
//			auto* breakableController = Breakable[i]->GetComponent<BreakableTileController>();
//			if (breakableController && breakableController->DoIBreak(inoutPlayer.IsJumping(), inoutPlayer.IsGroundPounding()))
//			{
//				VfxService::SpawnWorldEffect("block_destroy", Breakable[i]->GetTransform().GetPosition(), 1.0f, playerForwardSign);
//				inoutPlayer.SetCollisionSide(CollisionSide::Top, 0.0f);
//			}
//		}
//	}
//
//	for (size_t i = 0; i < checkThese.size(); i++)
//	{
//		const AABB3D<float> obstacleAABB = GetEffectiveAABB(*checkThese[i]);
//
//		const float distanceToEdge = 10.0f;
//
//		Ray<float> rayOne(Vector3f(playerCenter.x, playerMin.y, playerCenter.z), -Vector3f::UnitY);
//		float distanceOne = 0.0f;
//
//		if ((IntersectionAABBRay(obstacleAABB, rayOne, distanceOne)))
//		{
//			if (std::abs(distanceOne) < distanceToEdge)
//			{
//				if (inoutPlayer.GetVerticalVelocity() <= 0.1f)
//				{
//					if (checkThese[i]->GetComponent<HoverComponent>() != nullptr)
//					{
//						const float impactForce = std::fabs(inoutPlayer.GetVerticalVelocity());
//						inoutPlayer.NotifyHoverLanding(checkThese[i], impactForce, inoutPlayer.IsGroundPounding());
//					}
//
//					inoutPlayer.SetCollisionSide(CollisionSide::Bottom, 0.0f);
//
//					Vector3f diff = {
//						0.0f,
//						obstacleAABB.GetMax().y - playerMin.y - 0.1f,
//						0.0f
//					};
//					inoutPlayer.Nudge(diff);
//
//					anyCollision = true;
//					bottomCollision = true;
//				}
//			}
//		}
//
//		rayOne.InitWithOriginAndDirection(Vector3f(playerCenter.x, playerMax.y, playerCenter.z), Vector3f::UnitY);
//		distanceOne = 0.0f;
//
//		if ((IntersectionAABBRay(obstacleAABB, rayOne, distanceOne)))
//		{
//			if (std::abs(distanceOne) < distanceToEdge)
//			{
//				inoutPlayer.SetCollisionSide(CollisionSide::Top, 0.0f);
//
//				Vector3f diff = {
//					0.0f,
//					obstacleAABB.GetMin().y - playerMax.y,
//					0.0f
//				};
//				inoutPlayer.Nudge(diff);
//
//				anyCollision = true;
//			}
//		}
//
//		rayOne.InitWithOriginAndDirection(Vector3f(playerMax.x, playerCenter.y, playerCenter.z), Vector3f::UnitX);
//		distanceOne = 0.0f;
//
//		if ((IntersectionAABBRay(obstacleAABB, rayOne, distanceOne)))
//		{
//			if (std::abs(distanceOne) < distanceToEdge)
//			{
//				inoutPlayer.SetCollisionSide(CollisionSide::Right, 0.0f);
//
//				Vector3f diff = {
//					obstacleAABB.GetMin().x - playerMax.x - 10.0f, // this -10.0f prevent the game from breaking, don't question it :P
//					0.0f,
//					0.0f
//				};
//				inoutPlayer.Nudge(diff);
//
//				anyCollision = true;
//				rightCollision = true;
//			}
//		}
//
//		rayOne.InitWithOriginAndDirection(Vector3f(playerMin.x, playerCenter.y, playerCenter.z), -Vector3f::UnitX);
//		distanceOne = 0.0f;
//
//		if ((IntersectionAABBRay(obstacleAABB, rayOne, distanceOne)))
//		{
//			if (std::abs(distanceOne) < distanceToEdge)
//			{
//				inoutPlayer.SetCollisionSide(CollisionSide::Left, 0.0f);
//
//				Vector3f diff = {
//					obstacleAABB.GetMax().x - playerMin.x,
//					0.0f,
//					0.0f
//				};
//				inoutPlayer.Nudge(diff);
//
//				anyCollision = true;
//				leftCollision = true;
//			}
//		}
//	}
//
//	if (!bottomCollision)
//	{
//		inoutPlayer.ResetCollision(CollisionSide::Bottom);
//	}
//
//	if (!rightCollision)
//	{
//		inoutPlayer.ResetCollision(CollisionSide::Right);
//	}
//
//	if (!leftCollision)
//	{
//		inoutPlayer.ResetCollision(CollisionSide::Left);
//	}
//
//	if (!rightCollision && !leftCollision)
//	{
//		inoutPlayer.ResetStickyWallGrab();
//	}
//
//	return anyCollision;
//}
//
//// ---------------------------------------------------------------------------
//bool CollisionHandler::PlayerAgainstOne(PlayerController& inoutPlayer, GameObject& inoutGameObject)
//{
//	const AABB3D<float> playerAABB = GetEffectiveAABB(*inoutPlayer.GetOwner());
//	const AABB3D<float> enemyAABB = GetEffectiveAABB(inoutGameObject);
//
//	if (IntersectionAABBAABB(playerAABB, enemyAABB))
//	{
//		const Vector3<float>& playerPos = inoutPlayer.GetOwner()->GetTransform().GetPosition();
//
//		if (playerPos.y > enemyAABB.GetMax().y)
//		{
//			Vector3<float> bounceForce;
//			bounceForce.y = 1800.0f;
//			inoutPlayer.SetForce(bounceForce);
//			return true;
//		}
//	}
//	return false;
//}
//
//// ---------------------------------------------------------------------------
//bool CollisionHandler::SchnozAgainstMany(GameObject& inoutSchnoz, std::vector<GameObject*>& inoutObjectList)
//{
//	bool anyCollision = false;
//
//	SchnozController& schnoz = *inoutSchnoz.GetComponent<SchnozController>();
//	const AABB3D<float>& schnozAABB = schnoz.GetOwner()->GetComponent<BoxColliderComponent>()->GetAABB();
//	const Vector3f& schnozMin = schnozAABB.GetMin();
//	const Vector3f& schnozMax = schnozAABB.GetMax();
//	const Vector3f schnozCenter = (schnozMin + schnozMax) * 0.5f;
//
//	std::vector<GameObject*> checkThese = {};
//
//	{
//		const AABB3D<float> wideAreaBox = {
//			Vector3f(schnozMin.x - 500, schnozMin.y - 500, schnozMin.z - 500),
//			Vector3f(schnozMax.x + 500, schnozMax.y + 500, schnozMax.z + 500)
//		};
//		for (size_t i = 0; i < inoutObjectList.size(); i++)
//		{
//			if (IntersectionAABBAABB(wideAreaBox, GetEffectiveAABB(*inoutObjectList[i])))
//			{
//				checkThese.push_back(inoutObjectList[i]);
//			}
//		}
//	}
//
//	for (size_t i = 0; i < checkThese.size(); ++i)
//	{
//		const AABB3D<float> obstacleAABB = GetEffectiveAABB(*checkThese[i]);
//
//		const float distanceToEdge = 10.0f;
//
//		Ray<float> rayOne(Vector3f(schnozMax.x, schnozCenter.y, schnozCenter.z), Vector3f::UnitX);
//		float distanceOne = 0.0f;
//
//		if ((IntersectionAABBRay(obstacleAABB, rayOne, distanceOne)))
//		{
//			if (std::abs(distanceOne) < distanceToEdge)
//			{
//				schnoz.SetCollisionSide(CollisionSide::Right, 0.0f);
//
//				Vector3f diff = {
//					obstacleAABB.GetMin().x - schnozMax.x - 10.0f,
//					0.0f,
//					0.0f
//				};
//				schnoz.Nudge(diff);
//
//				anyCollision = true;
//			}
//		}
//
//		rayOne.InitWithOriginAndDirection(Vector3f(schnozMin.x, schnozCenter.y, schnozCenter.z), -Vector3f::UnitX);
//		distanceOne = 0.0f;
//
//		if ((IntersectionAABBRay(obstacleAABB, rayOne, distanceOne)))
//		{
//			if (std::abs(distanceOne) < distanceToEdge)
//			{
//				schnoz.SetCollisionSide(CollisionSide::Left, 0.0f);
//
//				Vector3f diff = {
//					obstacleAABB.GetMax().x - schnozMin.x,
//					0.0f,
//					0.0f
//				};
//				schnoz.Nudge(diff);
//
//				anyCollision = true;
//			}
//		}
//
//		rayOne.InitWithOriginAndDirection(Vector3f(schnozCenter.x, schnozMin.y, schnozCenter.z), -Vector3f::UnitY);
//		distanceOne = 0.0f;
//
//		if ((IntersectionAABBRay(obstacleAABB, rayOne, distanceOne)))
//		{
//			if (std::abs(distanceOne) < distanceToEdge)
//			{
//				schnoz.SetCollisionSide(CollisionSide::Bottom, 0.0f);
//
//				Vector3f diff = {
//						0.0f,
//						obstacleAABB.GetMax().y - schnozMin.y,
//						0.0f
//				};
//				schnoz.Nudge(diff);
//
//				anyCollision = true;
//			}
//		}
//	}
//
//	if (anyCollision)
//	{
//		inoutSchnoz.GetComponent<SchnozController>()->ChangeDirection();
//		return true;
//	}
//	else
//	{
//		inoutSchnoz.GetComponent<SchnozController>()->SetFalling();
//		return false;
//	}
//}
//
//// ---------------------------------------------------------------------------
//bool CollisionHandler::SchnozCheckIfAtEdge(GameObject& inoutSchnoz, std::vector<GameObject*>& inoutObjectList)
//{
//	bool anyCollision = false;
//	for (size_t i = 0; i < inoutObjectList.size(); ++i)
//	{
//		const GameObject& obstacle = *inoutObjectList[i];
//		const AABB3D<float> obstacleAABB = GetEffectiveAABB(obstacle);
//		float maxHeight = 10.0f;
//		float currentHeight = 0.0f;
//
//		if (IntersectionAABBRay(obstacleAABB, inoutSchnoz.GetComponent<SchnozController>()->GetRay(), currentHeight))
//		{
//			if (currentHeight < maxHeight)
//			{
//				anyCollision = true;
//				break;
//			}
//			else
//			{
//				//inoutSchnoz.GetComponent<SchnozController>()->ChangeDirection();
//				anyCollision = false;
//			}
//		}
//	}
//
//	if (!anyCollision)
//	{
//		inoutSchnoz.GetComponent<SchnozController>()->ChangeDirection();
//	}
//
//	return anyCollision;
//}
//
//// ---------------------------------------------------------------------------
//bool CollisionHandler::SchnozAgainstSchnoz(std::vector<GameObject*>& inoutSchnozList1, std::vector<GameObject*>& inoutSchnozList2)
//{
//	bool anyCollision = false;
//	for (size_t i = 0; i < inoutSchnozList1.size(); ++i)
//	{
//		for (size_t j = 0; j < inoutSchnozList2.size(); ++j)
//		{
//			if ((*inoutSchnozList1[i]).GetComponent<SchnozController>()->GetOwner() == (*inoutSchnozList2[j]).GetComponent<SchnozController>()->GetOwner())
//			{
//				continue;
//			}
//
//			const AABB3D<float> aabb1 = (*inoutSchnozList1[i]).GetComponent<SchnozController>()->GetOwner()->GetComponent<BoxColliderComponent>()->GetAABB();
//			const AABB3D<float> aabb2 = (*inoutSchnozList2[j]).GetComponent<SchnozController>()->GetOwner()->GetComponent<BoxColliderComponent>()->GetAABB();
//
//
//			if (IntersectionAABBAABB(aabb1, aabb2))
//			{
//				anyCollision = true;
//				(*inoutSchnozList1[i]).GetComponent<SchnozController>()->ChangeDirection();
//				(*inoutSchnozList2[j]).GetComponent<SchnozController>()->ChangeDirection();
//			}
//		}
//	}
//
//	return anyCollision;
//}
//
//// ---------------------------------------------------------------------------
//bool CollisionHandler::PlayerAgainstSchnoz(PlayerController& inoutPlayer, GameObject& inoutSchnoz)
//{
//	if (auto* schnoz = inoutSchnoz.GetComponent<SchnozController>())
//	{
//		if (schnoz->IsPrimedForDestruction())
//		{
//			return false;
//		}
//	}
//
//	GameObject* SchnozID = &inoutSchnoz;
//	const AABB3D<float> playerAABB = GetEffectiveAABB(*inoutPlayer.GetOwner());
//	const AABB3D<float> schnozAABB = inoutSchnoz.GetComponent<SchnozController>()->GetOwner()->GetComponent<BoxColliderComponent>()->GetAABB();
//	float stompTolerance = 0;
//
//	if (IntersectionAABBAABB(playerAABB, schnozAABB))
//	{
//		if (inoutSchnoz.GetTag() == GameObjectTags::Schnoz_II)
//		{
//			stompTolerance = 40.0f;
//		}
//		else
//		{
//			stompTolerance = 100.0f;
//		}
//
//		const bool isStompFromAbove = playerAABB.GetMin().y >= (schnozAABB.GetMax().y - stompTolerance);
//
//		if (isStompFromAbove)
//		{
//			if (inoutSchnoz.GetComponent<SchnozController>()->DoesSchnozDies(inoutPlayer, inoutPlayer.IsFalling(), inoutPlayer.IsGroundPounding()))
//			{
//				inoutSchnoz.GetComponent<SchnozController>()->PrimeForDestruction();
//				return true;
//			}
//			else if (inoutSchnoz.GetTag() == GameObjectTags::Schnoz_III && inoutSchnoz.GetComponent<SchnozController>()->GetDefencePoints() == 0)
//			{
//				AudioManager::GetInstance().Play(EAudioSource::Schnoz_Armor_Break);
//			}
//
//			return false;
//		}
//		else
//		{
//			auto* playerDamageable = inoutPlayer.GetOwner()->GetComponent<DamageableComponent>();
//			if (playerDamageable)
//			{
//				const int previousHp = playerDamageable->GetCurrentHealth();
//				playerDamageable->TakeDamage(playerDamageable->GetDamagePerHit(), SchnozID);
//				const int currentHp = playerDamageable->GetCurrentHealth();
//
//				if (currentHp != previousHp)
//				{
//					const float playerForwardSign = inoutPlayer.IsFacingRight() ? 1.0f : -1.0f;
//					VfxService::SpawnWorldEffect("player_hit", inoutPlayer.GetOwner()->GetTransform().GetPosition(), 1.0f, playerForwardSign);
//
//					std::cout << "[Combat] Player hit by enemy '" << inoutSchnoz.GetName()
//						<< "'. HP: " << currentHp
//						<< "/" << playerDamageable->GetMaxHealth() << "\n";
//
//					gCurrentLife = currentHp;
//				}
//
//				if (playerDamageable->IsDead() == true)
//				{
//					gCurrentLife = 0;
//				}
//			}
//			return true;
//		}
//	}
//
//	return false;
//}
//
//bool CollisionHandler::PlayerAgainstBoss(PlayerController& inoutPlayer, GameObject& inoutBoss)
//{
//	GameObject* bossID = &inoutBoss;
//	const AABB3D<float> playerAABB = GetEffectiveAABB(*inoutPlayer.GetOwner());
//	BossAnimator* bossAnimator = inoutBoss.GetComponent<BossAnimator>();
//	const AABB3D<float> bossAABB = bossAnimator->GetOwner()->GetComponent<BoxColliderComponent>()->GetAABB();
//
//	if (bossAnimator->GetCurrentAnimationIndex() != bossAnimator->Dead)
//	{
//		if (IntersectionAABBAABB(playerAABB, bossAABB))
//		{
//			constexpr float stompTolerance = 50.0f;
//			const bool isStompFromAbove = playerAABB.GetMin().y >= (bossAABB.GetMax().y - stompTolerance);
//
//			if (isStompFromAbove) // Player is jumping on boss
//			{
//				inoutPlayer.SetForce(bossAnimator->GetKnockbackForce());
//
//				if (inoutPlayer.IsGroundPounding())
//				{
//					bossAnimator->BossTakeDamage();
//					inoutPlayer.ResetCollsionAfterAttack();
//					return true;
//				}
//				return false;
//			}
//			else
//			{
//				auto* playerDamageable = inoutPlayer.GetOwner()->GetComponent<DamageableComponent>();
//				if (playerDamageable)
//				{
//					const int previousHp = playerDamageable->GetCurrentHealth();
//					playerDamageable->TakeDamage(playerDamageable->GetDamagePerHit(), bossID);
//					const int currentHp = playerDamageable->GetCurrentHealth();
//
//					if (currentHp != previousHp)
//					{
//						const float playerForwardSign = inoutPlayer.IsFacingRight() ? 1.0f : -1.0f;
//						VfxService::SpawnWorldEffect("player_hit", inoutPlayer.GetOwner()->GetTransform().GetPosition(), 1.0f, playerForwardSign);
//
//						std::cout << "[Combat] Player hit by enemy '" << inoutBoss.GetName()
//							<< "'. HP: " << currentHp
//							<< "/" << playerDamageable->GetMaxHealth() << "\n";
//					}
//				}
//				return true;
//			}
//		}
//	}
//
//	return false;
//}
//
//bool CollisionHandler::PlayerAgainstPickups(PlayerController& inoutPlayer, CollectibleManager& inoutCollectibleManager, RankSystem* aRankSystem)
//{
//	bool anyCollision = false;
//	const std::vector<GameObject*>& pickupList = inoutCollectibleManager.GetItems();
//	const AABB3D<float> playerCollider = GetEffectiveAABB(*inoutPlayer.GetOwner());
//	const float playerForwardSign = inoutPlayer.IsFacingRight() ? 1.0f : -1.0f;
//
//	for (int i = 0; i < pickupList.size(); i++)
//	{
//		GameObject* pickup = pickupList[i];
//		if (!pickup || !pickup->IsActive())
//		{
//			continue;
//		}
//
//		bool hit = false;
//
//		if (auto* sphereCollider = pickup->GetComponent<SphereColliderComponent>())
//		{
//			hit = IntersectionSphereAABB(sphereCollider->GetSphere(), playerCollider);
//
//			if (!hit && sphereCollider->IsTrigger())
//			{
//				sphereCollider->OnTriggerExit();
//			}
//		}
//		else
//		{
//			const AABB3D<float> pickupCollider = GetEffectiveAABB(*pickup);
//			hit = IntersectionAABBAABB(playerCollider, pickupCollider);
//		}
//
//		if (hit)
//		{
//			anyCollision = true;
//
//			if (auto* sphereCollider = pickup->GetComponent<SphereColliderComponent>())
//			{
//				if (sphereCollider->IsTrigger())
//				{
//					sphereCollider->OnTriggerEnter();
//				}
//			}
//
//			const std::string& tag = pickup->GetTag();
//
//			if (tag == GameObjectTags::Coin)
//			{
//				AddCollectedCoin(1);
//				AudioManager::GetInstance().Stop(EAudioSource::Whisp_Pickup_SFX);
//				AudioManager::GetInstance().Play(EAudioSource::Whisp_Pickup_SFX);
//				aRankSystem->AddPoints(EPoints::Collecting_Wisps);
//				VfxService::SpawnWorldEffect("soul_pickup", pickup->GetTransform().GetPosition(), 1.0f, playerForwardSign);
//			}
//			else if (tag == GameObjectTags::Key)
//			{
//				AudioManager::GetInstance().Play(EAudioSource::Key_Collected_SFX);
//				aRankSystem->AddPoints(EPoints::Collecting_Keys);
//				AddCollectedKeys(1);
//
//				VfxService::SpawnWorldEffect("key_pickup", pickup->GetTransform().GetPosition(), 1.0f, playerForwardSign);
//			}
//			else if (tag == GameObjectTags::PowerUp)
//			{
//				if (auto* component = inoutPlayer.GetOwner()->GetComponent<DamageableComponent>())
//				{
//					component->Heal(1);
//					gCurrentLife = min(2, gCurrentLife + 1);
//					AudioManager::GetInstance().Play(EAudioSource::Power_Up_Collected_SFX);
//					VfxService::SpawnWorldEffect("heart_pickup", pickup->GetTransform().GetPosition(), 1.0f, playerForwardSign);
//				}
//			}
//
//			pickup->SetActive(false);
//			inoutCollectibleManager.RemoveItem(pickup);
//		}
//	}
//
//	return anyCollision;
//}
//
//void CollisionHandler::AddCollectedCoin(int anAmount)
//{
//	if (anAmount <= 0)
//	{
//		return;
//	}
//
//	gCollectedCoins += anAmount;
//}
//
//int CollisionHandler::GetCollectedCoins()
//{
//	return gCollectedCoins;
//}
//
//void CollisionHandler::ResetCollectedCoins()
//{
//	gCollectedCoins = 0;
//}
//
//int CollisionHandler::GetCurrentHealth()
//{
//	return gCurrentLife;
//}
//
//void CollisionHandler::ResetCurrentHealth()
//{
//	gCurrentLife = 2;
//}
//
//void CollisionHandler::AddCollectedKeys(int anAmount)
//{
//	if (anAmount <= 0)
//	{
//		return;
//	}
//
//	gCollectedKeys += anAmount;
//}
//
//int CollisionHandler::GetCollectedKeys()
//{
//	return gCollectedKeys;
//}
//
//void CollisionHandler::ResetCollectedKeys()
//{
//	gCollectedKeys = 0;
//}
//
//bool CollisionHandler::PlayerAgainstTriggerBox(PlayerController& inoutPlayer, std::vector<GameObject*>& inoutManager)
//{
//	bool anyCollision = false;
//
//	const AABB3D<float> playerCollider = GetEffectiveAABB(*inoutPlayer.GetOwner());
//
//	for (int i = 0; i < inoutManager.size(); i++)
//	{
//		const AABB3D<float> triggerCollider = GetEffectiveAABB(*inoutManager[i]);
//		auto* boxTrigger = inoutManager[i]->GetComponent<BoxColliderComponent>();
//
//		if (!boxTrigger)
//		{
//			continue;
//		}
//
//		if (IntersectionAABBAABB(playerCollider, triggerCollider))
//		{
//			anyCollision = true;
//			if (boxTrigger->IsTrigger())
//			{
//				boxTrigger->OnTriggerEnter();
//			}
//		}
//		else
//		{
//			if (boxTrigger->IsTrigger())
//			{
//				boxTrigger->OnTriggerExit();
//			}
//		}
//	}
//
//	return anyCollision;
//}
//
//bool CollisionHandler::PlayerAgainstDeathBoxes(PlayerController& inoutPlayer, std::vector<GameObject*>& inoutDeathBoxes)
//{
//	const AABB3D<float> playerCollider = GetEffectiveAABB(*inoutPlayer.GetOwner());
//
//	for (int i = 0; i < inoutDeathBoxes.size(); i++)
//	{
//		const AABB3D<float> triggerCollider = GetEffectiveAABB(*inoutDeathBoxes[i]);
//		auto* boxTrigger = inoutDeathBoxes[i]->GetComponent<BoxColliderComponent>();
//
//		if (!boxTrigger)
//		{
//			continue;
//		}
//
//		if (IntersectionAABBAABB(playerCollider, triggerCollider))
//		{
//			if (auto* damageComponent = inoutPlayer.GetOwner()->GetComponent<DamageableComponent>())
//			{
//				if (!damageComponent->IsDead())
//				{
//					damageComponent->SetCurrentHealth(0);
//				}
//			}
//			return true;
//		}
//	}
//
//	return false;
//}
