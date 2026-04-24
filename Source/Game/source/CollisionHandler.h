#pragma once
#include <vector>
#include <memory>

class GameObject;
class PlayerController;
class SchnozController;
class Schnoz_III;
class CollectibleManager;
class AudioManager;
class RankSystem;

/// Utility class for collision detection using CommonUtilities::Intersection.
/// Supports BoxColliderComponent (AABB) and SphereColliderComponent colliders.
class CollisionHandler
{
	public:
	CollisionHandler() = default;
	~CollisionHandler() = default;

	/// Takes in one player object and checks if it collides with anything in the object list.
	static bool PlayerAgainstMany(PlayerController& inoutPlayer, std::vector<GameObject*>& inoutObjectList);

	/// Takes in a player object and checks if it collides with the other object.
	static bool PlayerAgainstOne(PlayerController& inoutPlayer, GameObject& inoutGameObject);

	/// Takes in one Schnoz and checks if it collides with anything in the object list.
	static bool SchnozAgainstMany(GameObject& inoutSchnoz, std::vector<GameObject*>& inoutObjectList);

	/// Uses AABB-Ray test to check if the Schnoz is at the edge of a platform.
	static bool SchnozCheckIfAtEdge(GameObject& inoutSchnoz, std::vector<GameObject*>& inoutObjectList);

	/// Checks collisions between two lists of Schnoz enemies using AABB-AABB tests.
	static bool SchnozAgainstSchnoz(std::vector<GameObject*>& inoutSchnozList1, std::vector<GameObject*>& inoutSchnozList2);

	/// Takes in one Schnoz and one player to checks if it collides with each other.
	static bool PlayerAgainstSchnoz(PlayerController& inoutPlayer, GameObject& inoutSchnoz);

	static bool PlayerAgainstBoss(PlayerController& inoutPlayer, GameObject& inoutBoss);

	/// Checks if the player is colliding with any pickups.
	static bool PlayerAgainstPickups(PlayerController& inoutPlayer, CollectibleManager& inoutCollectibleManager, RankSystem* aRankSystem);
	
	/// Checks if the player is colliding in a TriggerBox 
	static bool PlayerAgainstTriggerBox(PlayerController& inoutPlayer, std::vector<GameObject*>& inoutManager);

	static bool PlayerAgainstDeathBoxes(PlayerController& inoutPlayer, std::vector<GameObject*>& inoutDeathBoxes);

	/// Global runtime count of collected coins.
	static void AddCollectedCoin(int anAmount = 1);
	static int GetCollectedCoins();
	static void ResetCollectedCoins();

	/// Global runtime count of health
	static int GetCurrentHealth();
	static void ResetCurrentHealth();

	/// Global runtime count of collected keys.
	static void AddCollectedKeys(int anAmount = 1);
	static int GetCollectedKeys();
	static void ResetCollectedKeys();

	private:
};