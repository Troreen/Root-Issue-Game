#include "PlayerState_Walk.h"
#include <CommonUtilities/Transform.hpp>
#include <CommonUtilities/AABB3D.hpp>
#include "Essentials/Essentials.h"
#include "GameObject.h"
#include "PlayerState_Master.h"
#include "PlayerControllerComponent.h"
#include "BulletComponent.h"
#include <memory>
#include "AnimationGraphComponent.h"


PlayerState_Walk::PlayerState_Walk()
{
	myWalkSpeed = 300.f;
}

void PlayerState_Walk::Update(float aDeltaTime, PlayerControllerComponent& aPlayerController)
{
	aPlayerController;
	CommonUtilities::Vector3<float> direction;
	CommonUtilities::Vector3<float> forwardAxis = Essentials::globalCamera.get()->GetCamera().GetTransform().GetForward();
	forwardAxis.y = 0;
	forwardAxis.Normalize();
	CommonUtilities::Vector3<float> rightAxis = Essentials::globalCamera.get()->GetCamera().GetTransform().GetRight();

	float myWalkAnimation = 0.f;

	if (Essentials::globalInputManager.get()->IsKeyHeld(static_cast<int>(Keys::W)))
	{
		direction += forwardAxis;
	}
	if (Essentials::globalInputManager.get()->IsKeyHeld(static_cast<int>(Keys::A)))
	{
		direction -= rightAxis;
	}
	if (Essentials::globalInputManager.get()->IsKeyHeld(static_cast<int>(Keys::S)))
	{
		direction -= forwardAxis;
	}
	if (Essentials::globalInputManager.get()->IsKeyHeld(static_cast<int>(Keys::D)))
	{
		direction += rightAxis;
	}

	GameObject* player = Essentials::GetEssentials().GetPlayer();
	direction = direction.GetNormalized() * myWalkSpeed * aDeltaTime;

	if (direction.LengthSqr() > 0)
	{
		myWalkAnimation = 1.f;
		player->GetTransform().Translate(direction);
		player->GetTransform().SetYawPitchRollRadians({ std::atan2f(direction.x, direction.z), 0, 0 });
	}
	
	if (Essentials::globalInputManager.get()->IsKeyHeld(static_cast<int>(Keys::SPACE)))
	{
		GameObject& bullet = aPlayerController.GetBullet();

		bullet.GetComponent<BulletComponent>()->SetSpeedDirectionPosition(100, direction, aPlayerController.GetOwner()->GetTransform().GetPosition());
		
	}


	aPlayerController.GetOwner()->GetComponent<AnimationGraphComponent>()->SetFloatParameter("w_walk", myWalkAnimation);
	const float offset = 1000.f;
	const float yaw = 45.0f * 3.14159265359f / 180.0f;
	const float pitch = 35.0f * 3.14159265359f / 180.0f;

	CommonUtilities::Quaternion<float> targetRotation = CommonUtilities::Quaternion<float>::CreateFromYawPitchRoll(yaw, pitch, 0);

	/*CommonUtilities::Quaternion<float> oldRotation = Essentials::globalCamera->GetCamera().GetTransform().GetRotation();

	CommonUtilities::Quaternion<float> newRotation = oldRotation * targetRotation;*/

	Essentials::globalCamera->SetCameraTransformFromScene(CommonUtilities::Vector3<float>{ -offset, offset, -offset } + player->GetTransform().GetPosition(), targetRotation);
}

