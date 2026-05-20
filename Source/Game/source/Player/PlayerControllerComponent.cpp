#include "PlayerControllerComponent.h"
#include "Essentials/Essentials.h"
#include <tge/math/Vector.h>
#include "GameObject.h"
#include <tge/math/Matrix.h>
#include "PlayerState_Walk.h"
#include "PlayerState_Master.h"
#include "AnimationGraphComponent.h"
#include "MeshComponent.h"
#include "ParticleEmitterComponent.h"
#include "CapsuleColliderComponent.h"
#include "DamageableComponent.h"
#include "SceneObjectData.h"


#include <algorithm>
#include <cmath>
#include <utility>

PlayerControllerComponent::PlayerControllerComponent(const SceneObjectData& aData)
{
	PlayerState_Master::Instance().myWalkState->SetHasGun(aData.GetPropertyOr<bool>("colliderOffset", false));
	PlayerState_Master::Instance().myWalkState->SetHasGun(aData.GetPropertyOr<bool>("HasGun", true));
}

void PlayerControllerComponent::Reset()
{
	StopForcedMove();
	SetState(PlayerState_Master::Instance().myWalkState.get());
}

void PlayerControllerComponent::Save()
{
	DamageableComponent* component = GetOwner()->GetComponent<DamageableComponent>();
	component->SetCurrentHealth(component->GetMaxHealth());
}

void PlayerControllerComponent::OnStart()
{
	SetState(PlayerState_Master::Instance().myWalkState.get());
}

void PlayerControllerComponent::OnUpdate(float aDeltaTime)
{
	if (myForcedMoveActive)
	{
		UpdateForcedMove(aDeltaTime);
		return;
	}

	if (!myState)
	{
		return;
	}

	if (GetOwner()->GetComponent<DamageableComponent>()->IsDead() &&
		myState != PlayerState_Master::Instance().myDeathState.get())
	{
		SetState(PlayerState_Master::Instance().myDeathState.get());
	}

	myState->BindToOwner(GetOwner());
	myState->Update(aDeltaTime, *this);
}

void PlayerControllerComponent::SetState(PlayerState* aState)
{
	if (myState)
	{
		myState->ResetValues();
	}
	myState = aState;
	if (!myState)
	{
		return;
	}

	myState->BindToOwner(GetOwner());
	myState->SetValues();
}


void PlayerControllerComponent::FireBullet()
{
	std::unique_ptr<GameObject> bullet = std::make_unique<GameObject>("Bullet");
	bullet->SetLayer(ObjectLayer::Projectile);

	//bullet->AddComponent<MeshComponent>("animations/SK/SK_CH_Player.fbx");

	ParticleEmitterComponent* particle = bullet->AddComponent<ParticleEmitterComponent>();
	particle->AttachSettings();
	particle->SetEmissionDirection(ParticleType::EnergySmall, -GetOwner()->GetTransform().GetForward());
	// Set new offset based on mesh size in the future. 
	// There's now a SetOffset function for emittercomponent to be used in the future. 
	// For now, spawning offset can be set through VFX_emitter_settings.json
	particle->SetContinuousEmission(ParticleType::EnergySmall, true);

	CapsuleColliderComponent* collider = bullet->AddComponent<CapsuleColliderComponent>(20.f, 200.f);
	collider->SetIsTrigger(true);

	BulletComponent* component = bullet->AddComponent<BulletComponent>();
	component->SetTransform(GetOwner()->GetTransform());

	Essentials::AddGameObject(bullet);
}

void PlayerControllerComponent::EnableGun(bool aEnable)
{
	if (PlayerState_Walk* walk = PlayerState_Master::myWalkState.get())
	{
		walk->SetHasGun(aEnable);
	}
}

void PlayerControllerComponent::StartForcedMoveTo(
	const CommonUtilities::Vector3<float>& aTargetPosition,
	const float aSpeed,
	ForcedMoveCompleteCallback aOnComplete)
{
	myForcedMoveTarget = aTargetPosition;
	myForcedMoveSpeed = (std::max)(1.0f, aSpeed);
	myForcedMoveCompleteCallback = std::move(aOnComplete);
	myForcedMoveActive = true;
	SetWalkAnimation(1.0f);
}

void PlayerControllerComponent::StopForcedMove()
{
	myForcedMoveActive = false;
	myForcedMoveCompleteCallback = {};
	SetWalkAnimation(0.0f);
}

bool PlayerControllerComponent::IsForcedMoveActive() const
{
	return myForcedMoveActive;
}

void PlayerControllerComponent::FaceDirection(const CommonUtilities::Vector3<float>& aDirection)
{
	CommonUtilities::Vector3<float> direction = aDirection;
	direction.y = 0.0f;
	if (direction.LengthSqr() <= 0.0001f)
	{
		return;
	}

	direction.Normalize();
	GetOwner()->GetTransform().SetYawPitchRollRadians({ std::atan2f(direction.x, direction.z), 0.0f, 0.0f });
}

void PlayerControllerComponent::UpdateForcedMove(const float aDeltaTime)
{
	GameObject* owner = GetOwner();
	if (!owner)
	{
		StopForcedMove();
		return;
	}

	CommonUtilities::Vector3<float> position = owner->GetTransform().GetPosition();
	CommonUtilities::Vector3<float> toTarget = myForcedMoveTarget - position;
	toTarget.y = 0.0f;

	const float distance = toTarget.Length();
	const float step = myForcedMoveSpeed * aDeltaTime;
	if (distance <= step || distance <= 1.0f)
	{
		position.x = myForcedMoveTarget.x;
		position.z = myForcedMoveTarget.z;
		owner->GetTransform().SetPosition(position);
		myForcedMoveActive = false;
		SetWalkAnimation(0.0f);

		ForcedMoveCompleteCallback callback = std::move(myForcedMoveCompleteCallback);
		myForcedMoveCompleteCallback = {};
		if (callback)
		{
			callback();
		}
		return;
	}

	const CommonUtilities::Vector3<float> direction = toTarget / distance;
	owner->GetTransform().Translate(direction * step);
	FaceDirection(direction);
	SetWalkAnimation(1.0f);
}

void PlayerControllerComponent::SetWalkAnimation(const float aWeight)
{
	GameObject* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	if (auto* animationGraph = owner->GetComponent<AnimationGraphComponent>())
	{
		animationGraph->SetFloatParameter("w_walk", aWeight);
	}
}
