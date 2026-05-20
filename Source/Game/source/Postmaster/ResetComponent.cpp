#include "ResetComponent.h"
#include "Essentials/Essentials.h"
#include "GameObject.h"

ResetComponent::ResetComponent(const SceneObjectData& aResetData)
{
	Essentials::globalPostMaster->Subscribe(MessageType::ReloadScene, this);
	Essentials::globalPostMaster->Subscribe(MessageType::SaveScene, this);

	myResetData = aResetData;
}

ResetComponent::~ResetComponent()
{
	Essentials::globalPostMaster->Unsubscribe(MessageType::ReloadScene, this);
	Essentials::globalPostMaster->Unsubscribe(MessageType::SaveScene, this);
}

void ResetComponent::Receive(const Message& aMsg)
{
	switch (aMsg.myMessageType)
	{
	case MessageType::ReloadScene:
	{
		GameObject& object = *GetOwner();
		object.Reset();
		object.GetTransform().SetPosition(myResetData.position);
		object.GetTransform().SetRotation(myResetData.rotation);
		object.GetTransform().SetScale(myResetData.scale);
		break;
	}
	case MessageType::SaveScene:
	{
		GameObject& object = *GetOwner();
		object.Save();
		myResetData.position = object.GetTransform().GetPosition();
		myResetData.rotation = object.GetTransform().GetRotation();
		myResetData.scale = object.GetTransform().GetScale();
		break;
	}
	}
}
