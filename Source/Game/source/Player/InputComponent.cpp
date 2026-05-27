#include "InputComponent.h"
#include "Essentials/Essentials.h"
#include "MouseDirectionComponent.h"

void InputComponent::OnStart()
{
	myForwardAxis = Essentials::globalCamera.get()->GetCamera().GetTransform().GetForward();
	myForwardAxis.y = 0;
	myForwardAxis.Normalize();
	myRightAxis = Essentials::globalCamera.get()->GetCamera().GetTransform().GetRight();
}

void InputComponent::OnUpdate(float)
{
	myHasInput = false;
	myIsAttacking = false;
	myIsCharging = false;
	myIsWalking = false;
	myIsWalking = true;


	if (Essentials::globalInputManager.get()->IsKeyPressed(static_cast<int>(Keys::SPACE)))
	{
		myIsAttacking = true;
		myHasInput = true;
		return;
	}

	if (Essentials::globalInputManager.get()->IsKeyHeld(static_cast<int>(Keys::MOUSELBUTTON)))
	{
		myIsCharging = true;
		myHasInput = true;
	}

	UpdateWalk();
}

void InputComponent::UpdateWalk()
{
	myTentativeDirection.x = 0;
	myTentativeDirection.y = 0;
	myTentativeDirection.z = 0;

	if (Essentials::globalInputManager.get()->IsKeyHeld(static_cast<int>(Keys::W)))
	{
		myTentativeDirection += myForwardAxis;
		myHasInput = true;
		myIsWalking = true;
	}
	if (Essentials::globalInputManager.get()->IsKeyHeld(static_cast<int>(Keys::A)))
	{
		myTentativeDirection -= myRightAxis;
		myHasInput = true;
		myIsWalking = true;
	}
	if (Essentials::globalInputManager.get()->IsKeyHeld(static_cast<int>(Keys::S)))
	{
		myTentativeDirection -= myForwardAxis;
		myHasInput = true;
		myIsWalking = true;
	}
	if (Essentials::globalInputManager.get()->IsKeyHeld(static_cast<int>(Keys::D)))
	{
		myTentativeDirection += myRightAxis;
		myHasInput = true;
		myIsWalking = true;
	}

	myTentativeDirection.Normalize();

	if (myIsWalking)
	{
		myFacingDirection = myTentativeDirection;
	}
}

const CommonUtilities::Vector3<float>& InputComponent::GetFacingDirection()
{
	return myFacingDirection;
}

bool InputComponent::HasInput()
{
	return myHasInput;
}
