#include "ResetComponent.h"
#include "Essentials/Essentials.h"
#include "GameObject.h"

ResetComponent::ResetComponent(const SceneObjectData& aResetData)
{
	Essentials::globalPostMaster->Subscribe(MessageType::ReloadScene, this);

	myResetData = aResetData;
}

ResetComponent::~ResetComponent()
{
	Essentials::globalPostMaster->Unsubscribe(MessageType::ReloadScene, this);
}

void ResetComponent::Receive(const Message&)
{
	GameObject& object = *GetOwner();
	object.Reset();
	object.GetTransform().SetPosition(myResetData.position);
	object.GetTransform().SetRotation(myResetData.rotation);
	object.GetTransform().SetScale(myResetData.scale);
}
