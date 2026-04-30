#pragma once

#include <unordered_map>
#include <vector>

#include <tge/script/ScriptRuntimeInstance.h>

namespace Tga
{
	enum class LivePreviewMode
	{
		Stopped,
		Paused,
		Running
	};

	struct LivePreviewData
	{
		LivePreviewMode mode;
		ScriptPinId pinToTrigger;
		int frameNumber;

		std::unordered_map<StringId, ModelSpacePose> poses;
		std::unordered_map<StringId, Property> dynamicProperties;
		std::unordered_map<StringId, Property> staticProperties;

		std::unordered_set<StringId> enabledScripts;
		std::vector<std::pair<StringId, ScriptRuntimeInstance>> scriptInstances;
	};
}
