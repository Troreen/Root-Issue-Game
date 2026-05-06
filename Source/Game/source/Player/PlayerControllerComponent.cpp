#include "PlayerControllerComponent.h"
#include "Essentials/Essentials.h"
#include <tge/math/Vector.h>
#include "GameObject.h"
#include <tge/math/Matrix.h>
#include "PlayerState_Walk.h"
#include "PlayerState_Master.h"



void PlayerControllerComponent::OnStart()
{
	const float yaw = 45.0f * 3.14159265359f / 180.0f;
	const float pitch = 45.0f * 3.14159265359f / 180.0f;

	const float dist = 3000;
	const float pitchOffset = 19 * 3.14159265359f / 180.0f;

	myCameraOffset.x = -dist;
	myCameraOffset.y = dist * (pitch + pitchOffset) / yaw;
	myCameraOffset.z = -dist;

	myPosition = GetOwner()->GetTransform().GetPosition();

	myCameraRotation = CommonUtilities::Quaternion<float>::CreateFromYawPitchRoll(yaw, pitch, 0);

	SetState(PlayerState_Master::Instance().myWalkState.get());

	Essentials::globalCamera->SetCameraTransformFromScene(myCameraOffset + myPosition, myCameraRotation);
	Tga::Vector2ui resolution = Tga::Engine::GetInstance()->GetRenderSize();
	//const float renderScalar = 1.6f;
	//Essentials::globalCamera->GetRenderCamera().SetOrtographicProjection((float)resolution.x * renderScalar, (float)resolution.y * renderScalar, 50000);
	Essentials::globalCamera->GetRenderCamera().SetPerspectiveProjection(23, { (float)resolution.x, (float)resolution.y }, 1, 10000);
}

void PlayerControllerComponent::OnUpdate(float aDeltaTime)
{
	myState->Update(aDeltaTime, *this);

	for (auto& bullet : myBullets)
	{
		if (bullet->IsActive())
		{
			bullet->Update(aDeltaTime);
		}
	}

	myPosition = GetOwner()->GetTransform().GetPosition();

	Essentials::globalCamera->SetCameraTransformFromScene(myCameraOffset + myPosition, myCameraRotation);
}

void PlayerControllerComponent::Render()
{
	for (auto& bullet : myBullets)
	{
		if (bullet->IsActive())
		{
			bullet->Render();
		}
	}
}


void PlayerControllerComponent::SetBullet(std::unique_ptr<GameObject> aBullet)
{
	myBullets.push_back(std::move(aBullet));
}

void PlayerControllerComponent::SetState(PlayerState* aState)
{
	myState = aState;
	myState->ResetValues();
}

