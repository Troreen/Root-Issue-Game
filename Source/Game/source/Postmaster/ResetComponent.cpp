#include "ResetComponent.h"
#include "Essentials/Essentials.h"
#include "GameObject.h"

ResetComponent::ResetComponent()
{
	Essentials::globalPostMaster->Subscribe(MessageType::ReloadScene, this);
}

ResetComponent::~ResetComponent()
{
	Essentials::globalPostMaster->Unsubscribe(MessageType::ReloadScene, this);
}

void ResetComponent::Receive(const Message&)
{
	GetOwner()->Reset();
}
