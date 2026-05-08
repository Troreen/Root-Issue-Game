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


#include <algorithm>
#include <cmath>
#include <utility>

void PlayerControllerComponent::Reset()
{
	StopForcedMove();
	SetState(PlayerState_Master::Instance().myWalkState.get());

	if (auto* animationGraph = GetOwner()->GetComponent<AnimationGraphComponent>())
	{
		animationGraph->SetFloatParameter("w_death", 0.f);
	}

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

	myState->BindToOwner(GetOwner());
	myState->Update(aDeltaTime, *this);
}

void PlayerControllerComponent::SetState(PlayerState* aState)
{
	myState = aState;
	if (!myState)
	{
		return;
	}

	myState->BindToOwner(GetOwner());
	myState->ResetValues();
}


void PlayerControllerComponent::FireBullet()
{
	std::unique_ptr<GameObject> bullet = std::make_unique<GameObject>("MeleeEnemy");

	//bullet->AddComponent<MeshComponent>("animations/SK/SK_CH_Player.fbx");

	ParticleEmitterComponent* particle = bullet->AddComponent<ParticleEmitterComponent>();
	particle->AttachSettings();
	particle->SetContinuousEmission(ParticleType::Blood, true);

	BulletComponent* component = bullet->AddComponent<BulletComponent>();
	component->SetTransform(GetOwner()->GetTransform());

	Essentials::AddGameObject(bullet);
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
