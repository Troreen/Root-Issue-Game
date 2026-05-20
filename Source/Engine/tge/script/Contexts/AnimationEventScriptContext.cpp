#include <stdafx.h>

#include "AnimationEventScriptContext.h"

#include <iostream>

void Tga::AnimationEventScriptContext::LogAnimationEvent() const
{
	std::cout << "[AnimationEventScript] event=" << eventId.GetString()
		<< " script=" << scriptId.GetString()
		<< " clip=" << clipPath.GetString()
		<< " time=" << eventTime
		<< "\n";
}
