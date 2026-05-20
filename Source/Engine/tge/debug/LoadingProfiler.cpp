#include "stdafx.h"
#include "LoadingProfiler.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

namespace Tga
{
namespace
{
	constexpr std::size_t kMaxRowsToPrint = 8;

	template <typename T, typename Getter>
	void SortDescending(std::vector<T>& someItems, Getter aGetter)
	{
		std::sort(
			someItems.begin(),
			someItems.end(),
			[&](const T& aLeft, const T& aRight)
			{
				return aGetter(aLeft) > aGetter(aRight);
			});
	}
}

LoadingProfiler::Scope::Scope(const char* aName)
{
#ifndef _RETAIL
	myName = aName;
	myStartTime = std::chrono::steady_clock::now();
#else
	(void)aName;
#endif
}

LoadingProfiler::Scope::~Scope()
{
#ifndef _RETAIL
	const auto endTime = std::chrono::steady_clock::now();
	const double milliseconds = std::chrono::duration<double, std::milli>(endTime - myStartTime).count();
	LoadingProfiler::GetInstance().RecordPhase(myName ? myName : "Unnamed", milliseconds);
#endif
}

LoadingProfiler& LoadingProfiler::GetInstance()
{
	static LoadingProfiler instance;
	return instance;
}

void LoadingProfiler::BeginSceneLoad(const std::string& aScenePath)
{
#ifndef _RETAIL
	std::lock_guard<std::mutex> lock(myMutex);
	myReport = Report{};
	myReport.active = true;
	myReport.scenePath = aScenePath;
	myReport.startTime = std::chrono::steady_clock::now();
#else
	(void)aScenePath;
#endif
}

void LoadingProfiler::MarkAsyncLoadFinished()
{
#ifndef _RETAIL
	std::lock_guard<std::mutex> lock(myMutex);
	if (!myReport.active)
	{
		return;
	}

	const auto now = std::chrono::steady_clock::now();
	myReport.asyncMilliseconds = std::chrono::duration<double, std::milli>(now - myReport.startTime).count();
#endif
}

void LoadingProfiler::FinishSceneLoadAndPrint()
{
#ifndef _RETAIL
	Report report;
	double totalMilliseconds = 0.0;
	{
		std::lock_guard<std::mutex> lock(myMutex);
		if (!myReport.active)
		{
			return;
		}

		const auto now = std::chrono::steady_clock::now();
		totalMilliseconds = std::chrono::duration<double, std::milli>(now - myReport.startTime).count();
		report = myReport;
		myReport.active = false;
	}

	PrintReportLocked(report, totalMilliseconds);
#endif
}

void LoadingProfiler::RecordPhase(const std::string& aName, double aMilliseconds)
{
#ifndef _RETAIL
	std::lock_guard<std::mutex> lock(myMutex);
	if (!myReport.active)
	{
		return;
	}

	PhaseTiming& timing = myReport.phases[aName];
	timing.totalMilliseconds += aMilliseconds;
	++timing.count;
#else
	(void)aName;
	(void)aMilliseconds;
#endif
}

void LoadingProfiler::RecordLevelDataStats(std::size_t aFileCount, std::uintmax_t aByteCount)
{
#ifndef _RETAIL
	std::lock_guard<std::mutex> lock(myMutex);
	if (!myReport.active)
	{
		return;
	}

	myReport.levelDataFileCount = aFileCount;
	myReport.levelDataByteCount = aByteCount;
#else
	(void)aFileCount;
	(void)aByteCount;
#endif
}

void LoadingProfiler::RecordSceneCacheStatus(const std::string& aStatus)
{
#ifndef _RETAIL
	std::lock_guard<std::mutex> lock(myMutex);
	if (!myReport.active)
	{
		return;
	}

	myReport.sceneCacheStatus = aStatus;
#else
	(void)aStatus;
#endif
}

void LoadingProfiler::RecordObjectDefinitionStats(std::size_t aFileCount, std::uintmax_t aByteCount)
{
#ifndef _RETAIL
	std::lock_guard<std::mutex> lock(myMutex);
	if (!myReport.active)
	{
		return;
	}

	myReport.objectDefinitionFileCount = aFileCount;
	myReport.objectDefinitionByteCount = aByteCount;
#else
	(void)aFileCount;
	(void)aByteCount;
#endif
}

void LoadingProfiler::RecordObjectCount(std::size_t anObjectCount)
{
#ifndef _RETAIL
	std::lock_guard<std::mutex> lock(myMutex);
	if (myReport.active)
	{
		myReport.objectCount = anObjectCount;
	}
#else
	(void)anObjectCount;
#endif
}

void LoadingProfiler::RecordTag(const std::string& aTag)
{
#ifndef _RETAIL
	std::lock_guard<std::mutex> lock(myMutex);
	if (myReport.active)
	{
		++myReport.tagCounts[aTag.empty() ? "<empty>" : aTag];
	}
#else
	(void)aTag;
#endif
}

void LoadingProfiler::RecordModelPath(const std::string& aPath)
{
#ifndef _RETAIL
	if (aPath.empty())
	{
		return;
	}

	std::lock_guard<std::mutex> lock(myMutex);
	if (myReport.active)
	{
		myReport.modelPaths.insert(aPath);
	}
#else
	(void)aPath;
#endif
}

void LoadingProfiler::RecordTexturePath(const std::string& aPath)
{
#ifndef _RETAIL
	if (aPath.empty())
	{
		return;
	}

	std::lock_guard<std::mutex> lock(myMutex);
	if (myReport.active)
	{
		myReport.texturePaths.insert(aPath);
	}
#else
	(void)aPath;
#endif
}

void LoadingProfiler::RecordAnimationPath(const std::string& aPath)
{
#ifndef _RETAIL
	if (aPath.empty())
	{
		return;
	}

	std::lock_guard<std::mutex> lock(myMutex);
	if (myReport.active)
	{
		myReport.animationPaths.insert(aPath);
	}
#else
	(void)aPath;
#endif
}

void LoadingProfiler::RecordModelLoad(const std::string& aPath, double aMilliseconds, bool aCacheHit)
{
#ifndef _RETAIL
	std::lock_guard<std::mutex> lock(myMutex);
	if (myReport.active)
	{
		myReport.modelLoads.push_back({ aPath, aMilliseconds, aCacheHit });
	}
#else
	(void)aPath;
	(void)aMilliseconds;
	(void)aCacheHit;
#endif
}

void LoadingProfiler::RecordAnimationLoad(const std::string& aPath, double aMilliseconds, bool aCacheHit)
{
#ifndef _RETAIL
	std::lock_guard<std::mutex> lock(myMutex);
	if (myReport.active)
	{
		myReport.animationLoads.push_back({ aPath, aMilliseconds, aCacheHit });
	}
#else
	(void)aPath;
	(void)aMilliseconds;
	(void)aCacheHit;
#endif
}

void LoadingProfiler::RecordGameObjectInit(const std::string& aName, const std::string& aTag, double aMilliseconds)
{
#ifndef _RETAIL
	std::lock_guard<std::mutex> lock(myMutex);
	if (myReport.active)
	{
		myReport.objectInits.push_back({ aName, aTag, aMilliseconds });
	}
#else
	(void)aName;
	(void)aTag;
	(void)aMilliseconds;
#endif
}

void LoadingProfiler::RecordScriptStart(const std::string& aComponentName, double aMilliseconds)
{
#ifndef _RETAIL
	std::lock_guard<std::mutex> lock(myMutex);
	if (myReport.active)
	{
		myReport.scriptStarts.push_back({ aComponentName, aMilliseconds, false });
	}
#else
	(void)aComponentName;
	(void)aMilliseconds;
#endif
}

void LoadingProfiler::PrintReportLocked(const Report& aReport, double aTotalMilliseconds) const
{
	std::cout << std::fixed << std::setprecision(2);
	std::cout << "\n========== Loading Report ==========\n";
	std::cout << "Scene: " << aReport.scenePath << "\n";
	std::cout << "Total: " << aTotalMilliseconds << " ms";
	if (aReport.asyncMilliseconds > 0.0)
	{
		std::cout << " | Async ready: " << aReport.asyncMilliseconds << " ms";
	}
	std::cout << "\n";
	std::cout << "Objects: " << aReport.objectCount
		<< " | leveldata files: " << aReport.levelDataFileCount
		<< " (" << aReport.levelDataByteCount << " bytes)"
		<< " | tgo files scanned: " << aReport.objectDefinitionFileCount
		<< " (" << aReport.objectDefinitionByteCount << " bytes)\n";
	if (!aReport.sceneCacheStatus.empty())
	{
		std::cout << "Scene cache: " << aReport.sceneCacheStatus << "\n";
	}
	std::cout << "Unique assets: models=" << aReport.modelPaths.size()
		<< " textures=" << aReport.texturePaths.size()
		<< " animations=" << aReport.animationPaths.size() << "\n";

	std::vector<std::pair<std::string, PhaseTiming>> phases(aReport.phases.begin(), aReport.phases.end());
	SortDescending(phases, [](const auto& anEntry) { return anEntry.second.totalMilliseconds; });
	std::cout << "-- Phases --\n";
	for (const auto& [name, timing] : phases)
	{
		std::cout << "  " << name << ": " << timing.totalMilliseconds << " ms";
		if (timing.count > 1)
		{
			std::cout << " (" << timing.count << "x)";
		}
		std::cout << "\n";
	}

	std::vector<std::pair<std::string, std::size_t>> tags(aReport.tagCounts.begin(), aReport.tagCounts.end());
	SortDescending(tags, [](const auto& anEntry) { return anEntry.second; });
	std::cout << "-- Top Tags --\n";
	for (std::size_t i = 0; i < tags.size() && i < kMaxRowsToPrint; ++i)
	{
		std::cout << "  " << tags[i].first << ": " << tags[i].second << "\n";
	}

	auto modelLoads = aReport.modelLoads;
	SortDescending(modelLoads, [](const AssetTiming& anEntry) { return anEntry.milliseconds; });
	std::cout << "-- Slow Model Loads --\n";
	for (std::size_t i = 0; i < modelLoads.size() && i < kMaxRowsToPrint; ++i)
	{
		std::cout << "  " << modelLoads[i].milliseconds << " ms "
			<< (modelLoads[i].cacheHit ? "[cache] " : "[load] ")
			<< modelLoads[i].path << "\n";
	}

	auto animationLoads = aReport.animationLoads;
	SortDescending(animationLoads, [](const AssetTiming& anEntry) { return anEntry.milliseconds; });
	std::cout << "-- Slow Animation Loads --\n";
	for (std::size_t i = 0; i < animationLoads.size() && i < kMaxRowsToPrint; ++i)
	{
		std::cout << "  " << animationLoads[i].milliseconds << " ms "
			<< (animationLoads[i].cacheHit ? "[cache] " : "[load] ")
			<< animationLoads[i].path << "\n";
	}

	auto objectInits = aReport.objectInits;
	SortDescending(objectInits, [](const ObjectInitTiming& anEntry) { return anEntry.milliseconds; });
	std::cout << "-- Slow Object Init --\n";
	for (std::size_t i = 0; i < objectInits.size() && i < kMaxRowsToPrint; ++i)
	{
		std::cout << "  " << objectInits[i].milliseconds << " ms ["
			<< objectInits[i].tag << "] " << objectInits[i].name << "\n";
	}

	auto scriptStarts = aReport.scriptStarts;
	SortDescending(scriptStarts, [](const AssetTiming& anEntry) { return anEntry.milliseconds; });
	std::cout << "-- Slow Script OnStart --\n";
	for (std::size_t i = 0; i < scriptStarts.size() && i < kMaxRowsToPrint; ++i)
	{
		std::cout << "  " << scriptStarts[i].milliseconds << " ms " << scriptStarts[i].path << "\n";
	}
	std::cout << "====================================\n\n";
	std::cout << std::flush;
}
}
