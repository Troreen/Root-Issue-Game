#pragma once
#include "MessageType.h"
#include <any>
#include <string>
#include <tge/math/Vector.h>

struct Message
{
	MessageType myMessageType;
	std::any myData;
	bool myBool;
	int myInt;
	float myFloat;
	const char* myConstChar;
	std::string myString;
	Tga::Vector3f myVector3f;
	Tga::Vector2f myVector2f;

	//For debuging
	void* mySender = nullptr;
	//
};