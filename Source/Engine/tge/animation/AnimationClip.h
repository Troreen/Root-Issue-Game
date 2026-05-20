#pragma once

#include <tge/script/CopyOnWriteWrapper.h>
#include <tge/animation/Pose.h>
#include <tge/animation/Skeleton.h>
#include <tge/stringRegistry/StringRegistry.h>

#include <vector>

namespace Tga
{
	struct AnimationEventMarker
	{
		float time = 0.f;
		StringId id;
		StringId scriptId;
	};

	struct AnimationClip
	{
		StringId animationSourcePath;
		StringId previewModelPath;
		
		float startTime = 0.f;
		float endTime = 0.f;

		float playbackRate = 1.f;

		bool isLooping = false;

		bool isSyncronized = false;
		float cycleOffsetPercentage = 0.f;
		float cycleCount = 1.f;

		std::vector<AnimationEventMarker> events;
	};

	AnimationClip* GetAnimationClip(StringId path);
	AnimationClip* GetOrCreateAnimationClip(StringId path);
	void SaveAnimationClip(StringId path);
}
