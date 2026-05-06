#pragma once
#include "Subscriber.h"
#include <vector>

class PostMaster
{
public:
	
	void Subscribe(const MessageType aMsgType, Subscriber* aSubscriber);
	void Unsubscribe(MessageType aMsgType, Subscriber* aSubscriber);
	void SendMsg(const Message& aMessage);

private:

	PostMaster();
	~PostMaster();

	std::vector<std::vector<Subscriber*>> mySubscribers;
};

