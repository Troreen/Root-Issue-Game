#pragma once
#include "Subscriber.h"
#include <vector>

class PostMaster
{
public:
	PostMaster();
	~PostMaster();

	void Subscribe(const MessageType aMsgType, Subscriber* aSubscriber);
	void Unsubscribe(MessageType aMsgType, Subscriber* aSubscriber);
	void SendMsg(const Message& aMessage);

private:

	std::vector<std::vector<Subscriber*>> mySubscribers;
};

