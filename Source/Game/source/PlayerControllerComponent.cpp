#include "PlayerControllerComponent.h"
#include "Essentials/Essentials.h"
#include <tge/math/Vector.h>
#include "GameObject.h"
#include <tge/math/Matrix.h>
#include "PlayerState_Walk.h"
#include "PlayerState_Master.h"



void PlayerControllerComponent::OnStart()
{
	myState = PlayerState_Master::Instance().myWalkState.get();
}

void PlayerControllerComponent::OnUpdate(float aDeltaTime)
{
	myState->Update(aDeltaTime, *this);

	//if (myBullet.get()->IsActive())
	//{
	//	myBullet->Update(aDeltaTime);
	//}
}

void PlayerControllerComponent::Render()
{
	//if (myBullet.get()->IsActive())
	//{
	//	myBullet->Render();
	//}
}


void PlayerControllerComponent::SetBullet(std::shared_ptr<GameObject> aBullet)
{
	myBullet = aBullet;
}

GameObject& PlayerControllerComponent::GetBullet()
{
	return *myBullet;
}
