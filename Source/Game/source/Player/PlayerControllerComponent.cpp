#include "PlayerControllerComponent.h"
#include "Essentials/Essentials.h"
#include <tge/math/Vector.h>
#include "GameObject.h"
#include <tge/math/Matrix.h>
#include "PlayerState_Walk.h"
#include "PlayerState_Master.h"

void PlayerControllerComponent::OnStart()
{
	SetState(PlayerState_Master::Instance().myWalkState.get());
}

void PlayerControllerComponent::OnUpdate(float aDeltaTime)
{
	myState->Update(aDeltaTime, *this);
}

void PlayerControllerComponent::SetState(PlayerState* aState)
{
	myState = aState;
	myState->ResetValues();
}

