#include "PostMaster.h"

PostMaster::PostMaster()
{
	mySubscribers.resize(static_cast<int>(MessageType::COUNT));
}

PostMaster::~PostMaster()
{
	mySubscribers.clear();
}

void PostMaster::Subscribe(const MessageType aMsgType, Subscriber* aSubscriber)
{
	auto& list = mySubscribers[static_cast<int>(aMsgType)];

	if (std::find(list.begin(), list.end(), aSubscriber) == list.end())
	{
		list.push_back(aSubscriber);
	}
}

void PostMaster::Unsubscribe(MessageType aMsgType, Subscriber* aSubscriber)
{
	auto& list = mySubscribers[static_cast<int>(aMsgType)];

	list.erase(std::remove(list.begin(), list.end(), aSubscriber), list.end());
}

void PostMaster::SendMsg(const Message& aMessage)
{
	auto& list = mySubscribers[static_cast<int>(aMessage.myMessageType)];

	auto subscriberCopy = list;

	for (Subscriber* subscriber : subscriberCopy)
	{
		subscriber->Receive(aMessage);
	}
}