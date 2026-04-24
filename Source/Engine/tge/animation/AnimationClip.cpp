#include <stdafx.h>
#include "AnimationClip.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include <tge/settings/settings.h>

using namespace Tga;


static std::unordered_map<StringId, AnimationClip> locLoadedClips;

AnimationClip* Tga::GetAnimationClip(StringId path)
{
	if (path.IsEmpty())
		return nullptr;

	auto it = locLoadedClips.find(path);
	if (it != locLoadedClips.end())
		return &it->second;

	std::filesystem::path resolvedPath = Tga::Settings::GameAssetRoot() / path.GetString();
	if (!fs::exists(resolvedPath))
		return nullptr;

	AnimationClip& clip = locLoadedClips[path];

	std::ifstream file(resolvedPath, std::ios::in);
	nlohmann::json json;
	file >> json;
	file.close();

	clip.animationSourcePath = StringRegistry::RegisterOrGetString(json.value("animation_source_path", ""));
	clip.previewModelPath = StringRegistry::RegisterOrGetString(json.value("preview_model_path", ""));

	clip.startTime = json.value("start_time", 0.f);
	clip.endTime =json.value("end_time", 0.f);
	clip.playbackRate = json.value("playback_rate", 1.f);
	clip.isLooping = json.value("is_looping", false);

	clip.isSyncronized = json.value("is_sync", false);
	clip.cycleOffsetPercentage = json.value("cycle_offset", 0.f);
	clip.cycleCount = json.value("cycle_count", 1.f);

	clip.events.clear();
	const nlohmann::json eventsJson = json.value("events", nlohmann::json::array());
	if (eventsJson.is_array())
	{
		for (const nlohmann::json& eventJson : eventsJson)
		{
			if (!eventJson.is_object())
			{
				continue;
			}

			const float eventTime = eventJson.value("time", -1.f);
			const std::string eventIdText = eventJson.value("id", "");
			if (eventIdText.empty() || !std::isfinite(eventTime))
			{
				continue;
			}

			AnimationEventMarker marker;
			marker.time = eventTime;
			marker.id = StringRegistry::RegisterOrGetString(eventIdText.c_str());
			clip.events.push_back(marker);
		}

		std::sort(
			clip.events.begin(),
			clip.events.end(),
			[](const AnimationEventMarker& aLeft, const AnimationEventMarker& aRight)
			{
				return aLeft.time < aRight.time;
			});
	}

	return &clip;
}

AnimationClip* Tga::GetOrCreateAnimationClip(StringId path)
{
	AnimationClip* clip = GetAnimationClip(path);

	if (clip == nullptr)
		clip = &locLoadedClips[path];

	return clip;
}


void Tga::SaveAnimationClip(StringId path)
{
	std::filesystem::path resolvedPath = Tga::Settings::GameAssetRoot() / path.GetString();
	AnimationClip& clip = locLoadedClips[path];
	nlohmann::json events = nlohmann::json::array();
	for (const AnimationEventMarker& marker : clip.events)
	{
		if (marker.id.IsEmpty())
		{
			continue;
		}

		events.push_back({
			{ "time", marker.time },
			{ "id", marker.id.GetString() },
			});
	}

	nlohmann::json json = {
	{ "animation_source_path", clip.animationSourcePath.GetString()},
	{ "preview_model_path", clip.previewModelPath.GetString()},
	{ "start_time", clip.startTime},
	{ "end_time", clip.endTime},
	{ "playback_rate", clip.playbackRate},

	{ "is_looping", clip.isLooping},
	{ "is_sync", clip.isSyncronized},
	{ "cycle_offset", clip.cycleOffsetPercentage},
	{ "cycle_count", clip.cycleCount},
	{ "events", events },


	};

	std::ofstream fout(resolvedPath, std::ios::trunc);
	fs::permissions(resolvedPath, fs::perms::all);
	fout << json.dump(2);
}