#include "InputComponent.h"
#include "Essentials/Essentials.h"
#include "MouseDirectionComponent.h"
#include "GameObject.h"

void InputComponent::OnStart()
{
	myInput = Essentials::globalInputManager.get();
	myForwardAxis = Essentials::globalCamera.get()->GetCamera().GetTransform().GetForward();
	myForwardAxis.y = 0;
	myForwardAxis.Normalize();
	myRightAxis = Essentials::globalCamera.get()->GetCamera().GetTransform().GetRight();

	myMouse = GetOwner()->GetComponent<MouseDirectionComponent>();
}

void InputComponent::OnUpdate(float)
{
	myHasInput = false;
	myIsAttacking = false;
	myIsCharging = false;
	myIsWalking = false;
	myIsWalking = false;

	myTentativeDirection.x = 0;
	myTentativeDirection.y = 0;
	myTentativeDirection.z = 0;

	if (myInput->PressingPlayerAttack())
	{
		myIsAttacking = true;
		myHasInput = true;
		return;
	}

	UpdateWalk();
	UpdateAim();
}

void InputComponent::UpdateWalk()
{
	if (myInput->PressingPlayerMovingUp() && !myInput->LeftStickHeld())
	{
		myTentativeDirection += myForwardAxis;
		myHasInput = true;
		myIsWalking = true;
	}
	if (myInput->PressingPlayerMovingLeft() && !myInput->LeftStickHeld())
	{
		myTentativeDirection -= myRightAxis;
		myHasInput = true;
		myIsWalking = true;
	}
	if (myInput->PressingPlayerMovingDown() && !myInput->LeftStickHeld())
	{
		myTentativeDirection -= myForwardAxis;
		myHasInput = true;
		myIsWalking = true;
	}
	if (myInput->PressingPlayerMovingRight() && !myInput->LeftStickHeld())
	{
		myTentativeDirection += myRightAxis;
		myHasInput = true;
		myIsWalking = true;
	}

	if (myInput->LeftStickHeld())
	{
		myTentativeDirection += myInput->LeftStick().x * myRightAxis;
		myTentativeDirection += myInput->LeftStick().y * myForwardAxis;
		myHasInput = true;
		myIsWalking = true;
	}
	if (myIsWalking)
	{
		myFacingDirection = myTentativeDirection;
	}

	myFacingDirection.Normalize();
}

void InputComponent::UpdateAim()
{
	myAimingDirection = myFacingDirection;

	if (myInput->PressingPlayerAim())
	{
		myIsCharging = true;
		myHasInput = true;

		if (myInput->IsKeyHeld(static_cast<int>(Keys::MOUSELBUTTON)))
		{
			myAimingDirection = Tga::Vector3f(myMouse->GetWorldDirection().y, 0, myMouse->GetWorldDirection().x);
		}
	}
}

const CommonUtilities::Vector3<float>& InputComponent::GetTentativeDirection()
{
	return myTentativeDirection;
}

const CommonUtilities::Vector3<float>& InputComponent::GetFacingDirection()
{
	return myFacingDirection;
}

const CommonUtilities::Vector3<float>& InputComponent::GetAimingDirection()
{
	return myAimingDirection;
}

bool InputComponent::HasInput()
{
	return myHasInput;
}

bool InputComponent::IsWalking()
{
	return myIsWalking;
}

bool InputComponent::IsCharging()
{
	return myIsCharging;
}
