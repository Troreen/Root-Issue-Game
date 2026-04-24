#pragma once
#include "Message.h"

class Subscriber
{
public:

	virtual ~Subscriber() = default;
	virtual void Receive(const Message& aMsg) = 0;

};