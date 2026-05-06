#include "CameraComponent.h"
#include "GameObject.h"
#include "Essentials/Essentials.h"

void CameraComponent::OnStart()
{
	const float yaw = 45.0f * 3.14159265359f / 180.0f;
	const float pitch = 45.0f * 3.14159265359f / 180.0f;

	const float dist = 3000;
	const float pitchOffset = 19 * 3.14159265359f / 180.0f;

	myOffset.x = -dist;
	myOffset.y = dist * (pitch + pitchOffset) / yaw;
	myOffset.z = -dist;

	myPosition = GetOwner()->GetTransform().GetPosition();

	myCameraRotation = CommonUtilities::Quaternion<float>::CreateFromYawPitchRoll(yaw, pitch, 0);


	Essentials::globalCamera->SetCameraTransformFromScene(myOffset + myPosition, myCameraRotation);
	Tga::Vector2ui resolution = Tga::Engine::GetInstance()->GetRenderSize();
	//const float renderScalar = 1.6f;
	//Essentials::globalCamera->GetRenderCamera().SetOrtographicProjection((float)resolution.x * renderScalar, (float)resolution.y * renderScalar, 50000);
	Essentials::globalCamera->GetRenderCamera().SetPerspectiveProjection(23, { (float)resolution.x, (float)resolution.y }, 1, 10000);
}

void CameraComponent::OnUpdate(float aDeltaTime)
{
	aDeltaTime;
	myPosition = GetOwner()->GetTransform().GetPosition();

	Essentials::globalCamera->SetCameraTransformFromScene(myOffset + myPosition, myCameraRotation);
}
