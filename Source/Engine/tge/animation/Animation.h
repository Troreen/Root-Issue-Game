#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <tge/math/ScaleRotationTranslation.h>
#include <tge/animation/Pose.h>

namespace Tga
{
	struct Animation
	{
		std::vector<LocalSpacePose> Frames;

		// The animation length in frames.
		unsigned int Length;

		// The number of framer per second
		float FramesPerSecond;

		// The animation duration in seconds.
		float Duration;

		std::wstring Name;
	};
}

