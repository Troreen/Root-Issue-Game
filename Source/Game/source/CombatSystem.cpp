#include "CombatSystem.h"

#include "CollisionQuery.h"
#include "DamageableComponent.h"
#include "DebugSettings.h"
#include "GameObject.h"
#include "KnockbackComponent.h"

#include <tge/drawers/LineDrawer.h>
#include <tge/engine.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/math/Matrix4x4.h>
#include <tge/math/color.h>
#include <tge/primitives/LinePrimitive.h>
#include "SwitchComponent.h"
#include "CheckpointComponent.h"
#include "DestructibleComponent.h"

#include "Essentials.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
	using Vector3f = CommonUtilities::Vector3<float>;

	constexpr float kPi = 3.14159265358979323846f;
	constexpr int kSphereSegments = 32;

	Vector3f GetAttackCenter(const AttackData& anAttack)
	{
		if (anAttack.owner == nullptr)
		{
			return Vector3f::Zero;
		}

		auto& transform = anAttack.owner->GetTransform();
		return transform.GetPosition()
			+ transform.GetRight() * anAttack.localCenterOffset.x
			+ Vector3f::UnitY * anAttack.localCenterOffset.y
			+ transform.GetForward() * anAttack.localCenterOffset.z;
	}

	Vector3f GetAttackForward(const AttackData& anAttack)
	{
		if (anAttack.owner == nullptr)
		{
			return Vector3f::UnitZ;
		}

		Vector3f forward = anAttack.owner->GetTransform().GetForward();
		if (forward.LengthSqr() <= 0.001f)
		{
			return Vector3f::UnitZ;
		}

		return forward.GetNormalized();
	}

	float GetAttackRadius(const AttackData& anAttack)
	{
		if (anAttack.radius > 0.0f)
		{
			return anAttack.radius;
		}

		return (std::max)({ anAttack.size.x, anAttack.size.y, anAttack.size.z }) * 0.5f;
	}

	CollisionQuery::Shape MakeAttackShape(const AttackData& anAttack)
	{
		switch (anAttack.collisionShape)
		{
		case CollisionShapeType::Box:
			return CollisionQuery::MakeBoxShape(GetAttackCenter(anAttack), anAttack.size);
		case CollisionShapeType::Sphere:
		default:
			return CollisionQuery::MakeSphereShape(GetAttackCenter(anAttack), GetAttackRadius(anAttack));
		}
	}

	float GetMaxProjectionOntoAxis(
		const CollisionQuery::Shape& aShape,
		const Vector3f& anOrigin,
		const Vector3f& anAxis)
	{
		switch (aShape.type)
		{
		case CollisionShapeType::Sphere:
			return (aShape.center - anOrigin).Dot(anAxis) + aShape.radius;

		case CollisionShapeType::Capsule:
			return (std::max)(
				(aShape.segmentA - anOrigin).Dot(anAxis),
				(aShape.segmentB - anOrigin).Dot(anAxis)) + aShape.radius;

		case CollisionShapeType::Obb:
		{
			const float projectedRadius =
				std::abs(aShape.axes[0].Dot(anAxis)) * aShape.halfExtents.x +
				std::abs(aShape.axes[1].Dot(anAxis)) * aShape.halfExtents.y +
				std::abs(aShape.axes[2].Dot(anAxis)) * aShape.halfExtents.z;
			return (aShape.center - anOrigin).Dot(anAxis) + projectedRadius;
		}

		case CollisionShapeType::Box:
		default:
		{
			const Vector3f center = (aShape.bounds.GetMin() + aShape.bounds.GetMax()) * 0.5f;
			const Vector3f halfExtents = (aShape.bounds.GetMax() - aShape.bounds.GetMin()) * 0.5f;
			const float projectedRadius =
				std::abs(anAxis.x) * halfExtents.x +
				std::abs(anAxis.y) * halfExtents.y +
				std::abs(anAxis.z) * halfExtents.z;
			return (center - anOrigin).Dot(anAxis) + projectedRadius;
		}
		}
	}

	bool CanSphereAttackHitTarget(
		const AttackData& anAttack,
		const CollisionQuery::Shape& anAttackShape,
		const CollisionQuery::Shape& aTargetShape)
	{
		if (anAttack.collisionShape != CollisionShapeType::Sphere || !anAttack.onlyHitForwardHemisphere)
		{
			return true;
		}

		return GetMaxProjectionOntoAxis(aTargetShape, anAttackShape.center, GetAttackForward(anAttack)) >= 0.0f;
	}

	Vector3f GetKnockback(GameObject& anAttacker, GameObject& aTarget, float aStrength)
	{
		Vector3f direction = aTarget.GetTransform().GetPosition() - anAttacker.GetTransform().GetPosition();
		direction.y = 0.0f;
		if (direction.LengthSqr() <= 0.001f)
		{
			direction = anAttacker.GetTransform().GetForward();
			direction.y = 0.0f;
		}

		if (direction.LengthSqr() <= 0.001f)
		{
			return Vector3f::Zero;
		}

		return direction.GetNormalized() * aStrength;
	}

#ifndef _RETAIL
	void DrawDebugLine(Tga::LineDrawer& aDrawer, const Vector3f& aFrom, const Vector3f& aTo, const Tga::Color& aColor)
	{
		Tga::LinePrimitive line;
		line.fromPosition = { aFrom.x, aFrom.y, aFrom.z };
		line.toPosition = { aTo.x, aTo.y, aTo.z };
		line.color = aColor.AsVec4();
		aDrawer.Draw(line);
	}

	void BuildHemisphereBasis(const Vector3f& aForward, Vector3f& outRight, Vector3f& outUp)
	{
		const Vector3f worldUp = Vector3f::UnitY;
		outRight = worldUp.Cross(aForward);
		if (outRight.LengthSqr() <= 0.001f)
		{
			outRight = Vector3f::UnitX;
		}
		else
		{
			outRight = outRight.GetNormalized();
		}

		outUp = aForward.Cross(outRight);
		if (outUp.LengthSqr() <= 0.001f)
		{
			outUp = Vector3f::UnitY;
		}
		else
		{
			outUp = outUp.GetNormalized();
		}
	}

	void DrawDebugBox(Tga::LineDrawer& aDrawer, const CollisionQuery::Shape& aShape, const Tga::Color& aColor)
	{
		const Vector3f min = aShape.bounds.GetMin();
		const Vector3f max = aShape.bounds.GetMax();
		const Vector3f corners[8] =
		{
			{ min.x, min.y, min.z },
			{ max.x, min.y, min.z },
			{ max.x, min.y, max.z },
			{ min.x, min.y, max.z },
			{ min.x, max.y, min.z },
			{ max.x, max.y, min.z },
			{ max.x, max.y, max.z },
			{ min.x, max.y, max.z }
		};

		constexpr int edges[12][2] =
		{
			{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
			{ 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
			{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
		};

		for (const auto& edge : edges)
		{
			DrawDebugLine(aDrawer, corners[edge[0]], corners[edge[1]], aColor);
		}
	}

	void DrawDebugHemisphere(
		Tga::LineDrawer& aDrawer,
		const Vector3f& aCenter,
		float aRadius,
		const Vector3f& aForward,
		const Tga::Color& aColor)
	{
		Vector3f right;
		Vector3f up;
		BuildHemisphereBasis(aForward, right, up);

		Vector3f previousRim;
		for (int i = 0; i <= kSphereSegments; ++i)
		{
			const float angle = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(kSphereSegments);
			const Vector3f rim = aCenter +
				right * (std::cos(angle) * aRadius) +
				up * (std::sin(angle) * aRadius);

			if (i > 0)
			{
				DrawDebugLine(aDrawer, previousRim, rim, aColor);
			}

			previousRim = rim;
		}

		const Vector3f pole = aCenter + aForward * aRadius;
		for (int meridian = 0; meridian < 4; ++meridian)
		{
			const float meridianAngle = 0.5f * kPi * static_cast<float>(meridian);
			const Vector3f sideAxis =
				right * std::cos(meridianAngle) +
				up * std::sin(meridianAngle);

			Vector3f previousPoint = aCenter + sideAxis * aRadius;
			for (int i = 1; i <= kSphereSegments / 4; ++i)
			{
				const float t = static_cast<float>(i) / static_cast<float>(kSphereSegments / 4);
				const float angle = t * 0.5f * kPi;
				const Vector3f point = aCenter +
					sideAxis * (std::cos(angle) * aRadius) +
					aForward * (std::sin(angle) * aRadius);
				DrawDebugLine(aDrawer, previousPoint, point, aColor);
				previousPoint = point;
			}
		}

		DrawDebugLine(aDrawer, aCenter, pole, Tga::Color{ 1.0f, 0.95f, 0.2f, 1.0f });
	}
#endif
}

std::uint64_t CombatSystem::StartAttack(const AttackData& anAttack)
{
	if (anAttack.owner == nullptr || anAttack.activeDurationSeconds <= 0.0f || anAttack.damage <= 0)
	{
		return 0;
	}

	ActiveAttack activeAttack;
	activeAttack.id = myNextAttackId++;
	activeAttack.data = anAttack;
	activeAttack.remainingSeconds = anAttack.activeDurationSeconds;
	myActiveAttacks.push_back(std::move(activeAttack));

	std::cout << "[Combat] Started attack id=" << myActiveAttacks.back().id
		<< " owner='" << anAttack.owner->GetName() << "'"
		<< " damage=" << anAttack.damage
		<< " shape=" << (anAttack.collisionShape == CollisionShapeType::Sphere ? "sphere" : "box") << "\n";

	return myActiveAttacks.back().id;
}

void CombatSystem::Update(float aDeltaTime, std::vector<std::unique_ptr<GameObject>>& someObjects)
{
	myHitEventsThisFrame.clear();

	for (ActiveAttack& attack : myActiveAttacks)
	{
		if (attack.cancelled)
			continue;

		bool hasHit = false;

		if (attack.data.owner == nullptr || !attack.data.owner->IsActive())
		{
			attack.remainingSeconds = 0.0f;
			continue;
		}

		const CollisionQuery::Shape attackShape = MakeAttackShape(attack.data);

		for (std::unique_ptr<GameObject>& object : someObjects)
		{
			GameObject* target = object.get();
			if (target == nullptr || !target->IsActive() || target == attack.data.owner)
			{
				continue;
			}

			if (!attack.data.targetLayers.Contains(target->GetLayer()) || attack.hitTargets.find(target->GetCollisionId()) != attack.hitTargets.end() || !CollisionQuery::HasRuntimeCollider(*target))
			{
				continue;
			}

			CollisionQuery::RefreshRuntimeCollider(*target);
			const CollisionQuery::Shape targetShape = CollisionQuery::GetShape(*target);
			Vector3f separation;
			Vector3f normal;
			float penetration = 0.0f;
			if (!CollisionQuery::TryComputeSeparation(
				attackShape,
				targetShape,
				separation,
				normal,
				penetration) ||
				!CanSphereAttackHitTarget(attack.data, attackShape, targetShape))
			{
				continue;
			}

			attack.hitTargets.insert(target->GetCollisionId());
			hasHit = true;

			const Vector3f knockback = GetKnockback(*attack.data.owner, *target, attack.data.knockbackStrength);
			if (DamageableComponent* damageable = target->GetComponent<DamageableComponent>())
			{
				damageable->TakeDamage(attack.data.damage, attack.data.owner);
			}

			if (KnockbackComponent* knockbackReceiver = target->GetComponent<KnockbackComponent>())
			{
				knockbackReceiver->ApplyImpulse(knockback);
			}

			switch (target->GetLayer())
			{
			case ObjectLayer::Switch:
				if (SwitchComponent* switchComponent = target->GetComponent<SwitchComponent>())
				{
					switchComponent->Toggle();
				}
				break;
			case ObjectLayer::WorldDamageable:
				if (DesctructibleComponent* destructible = target->GetComponent<DesctructibleComponent>())
				{
					destructible->Toggle();
				}
				if (CheckpointComponent* checkPoint = target->GetComponent<CheckpointComponent>())
				{
					checkPoint->Toggle();
				}
				break;
			}


			HitEvent event;
			event.attackId = attack.id;
			event.attacker = attack.data.owner;
			event.target = target;
			event.damage = attack.data.damage;
			event.knockback = knockback;
			event.type = attack.data.type;
			myHitEventsThisFrame.push_back(event);

			std::cout << "[Combat] Hit attackId=" << attack.id
				<< " attacker='" << attack.data.owner->GetName() << "'"
				<< " target='" << target->GetName() << "'"
				<< " damage=" << attack.data.damage << "\n";

			if (target->GetName() == "Player")
			{
				Essentials::globalAudioManager->PlaySFX(SoundID::eGore);
			}
		}

		if (!attack.data.isContinuous)
		{
			attack.remainingSeconds -= aDeltaTime;
		}

		if (attack.data.isContinuous && hasHit == true)
		{
			attack.remainingSeconds = 0.0f;
		}
	}

	myActiveAttacks.erase(
		std::remove_if(
			myActiveAttacks.begin(),
			myActiveAttacks.end(),
			[](const ActiveAttack& anAttack)
			{
				return anAttack.remainingSeconds <= 0.0f || anAttack.cancelled;
			}),
		myActiveAttacks.end());
}

const std::vector<HitEvent>& CombatSystem::GetHitEventsThisFrame() const
{
	return myHitEventsThisFrame;
}

void CombatSystem::RenderDebug() const
{
#ifndef _RETAIL
	if (!GameDebugSettings::ShowCombatHitboxes())
	{
		return;
	}

	Tga::Engine* engine = Tga::Engine::GetInstance();
	if (engine == nullptr)
	{
		return;
	}

	auto& graphicsEngine = engine->GetGraphicsEngine();
	Tga::GraphicsStateStack& graphicsStateStack = graphicsEngine.GetGraphicsStateStack();
	Tga::LineDrawer& drawer = graphicsEngine.GetLineDrawer();

	graphicsStateStack.Push();
	graphicsStateStack.SetTransform(Tga::Matrix4x4f::CreateIdentityMatrix());

	for (const ActiveAttack& attack : myActiveAttacks)
	{
		if (attack.data.owner == nullptr || !attack.data.owner->IsActive())
		{
			continue;
		}

		const CollisionQuery::Shape shape = MakeAttackShape(attack.data);
		const Tga::Color color = attack.data.team == CombatTeam::Player
			? Tga::Color{ 1.0f, 0.45f, 0.05f, 1.0f }
		: Tga::Color{ 1.0f, 0.1f, 0.1f, 1.0f };

		if (attack.data.collisionShape == CollisionShapeType::Sphere)
		{
			DrawDebugHemisphere(drawer, shape.center, shape.radius, GetAttackForward(attack.data), color);
		}
		else
		{
			DrawDebugBox(drawer, shape, color);
		}
	}

	graphicsStateStack.Pop();
#endif
}

CombatSystem* CombatService::ourSystem = nullptr;

void CombatService::Set(CombatSystem* aSystem)
{
	ourSystem = aSystem;
}

CombatSystem* CombatService::Get()
{
	return ourSystem;
}

std::uint64_t CombatService::StartAttack(const AttackData& anAttack)
{
	if (ourSystem == nullptr)
	{
		return 0;
	}

	return ourSystem->StartAttack(anAttack);
}

bool CombatService::IsAttackActive(std::uint64_t anID)
{
	if (ourSystem == nullptr)
	{
		return false;
	}

	return ourSystem->IsAttackActive(anID);

}

bool CombatSystem::IsAttackActive(std::uint64_t anId) const
{
	for (const auto& attack : myActiveAttacks)
	{
		if (attack.id == anId)
		{
			return true;
		}
	}

	return false;
}

void CombatSystem::CancelAttack(std::uint64_t anId)
{
	for (auto& attack : myActiveAttacks)
	{
		if (attack.id == anId)
		{
			attack.cancelled = true;
			attack.remainingSeconds = 0.0f;
			attack.hitTargets.clear();
			return;
		}
	}
}

void CombatService::CancelAttack(std::uint64_t anID)
{
	if (ourSystem)
		ourSystem->CancelAttack(anID);
}
