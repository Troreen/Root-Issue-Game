#include "MouseDirectionComponent.h"
#include "PlayerControllerComponent.h"
#include "CommonUtilities/Transform.hpp"

void MouseDirectionComponent::OnStart()
{
	myResolution = Essentials::globalEngine->GetRenderSize();
	myInput = Essentials::globalInputManager.get();

	CommonUtilities::Transform<float> transform = Essentials::globalCamera.get()->GetCamera().GetTransform();
	Tga::Vector3f direction = transform.GetForward().ToTga();

	myCameraAngle = std::atan2f(direction.x, -direction.z);

	myCameraDownAngleScalar = 1.4f;
}

void MouseDirectionComponent::OnUpdate(float aDeltaTime)
{
	aDeltaTime;
	myMousePosition = myInput->GetMousePosition();

	Tga::Vector2f tempDirection;

	tempDirection.x = (myMousePosition.x - myResolution.x / 2) / myCameraDownAngleScalar;
	tempDirection.y = myMousePosition.y - myResolution.y / 2;

	tempDirection.Normalize();


	myWorldDirection.x = tempDirection.x * std::cos(myCameraAngle) - tempDirection.y * std::sin(myCameraAngle);
	myWorldDirection.y = tempDirection.x * std::sin(myCameraAngle) + tempDirection.y * std::cos(myCameraAngle);
}

const Tga::Vector2f& MouseDirectionComponent::GetWorldDirection()
{
	return myWorldDirection;
}
