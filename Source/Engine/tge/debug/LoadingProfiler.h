#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Tga
{
class LoadingProfiler
{
public:
	class Scope
	{
	public:
		explicit Scope(const char* aName);
		~Scope();

		Scope(const Scope&) = delete;
		Scope& operator=(const Scope&) = delete;

	private:
		const char* myName = nullptr;
		std::chrono::steady_clock::time_point myStartTime;
	};

	static LoadingProfiler& GetInstance();

	void BeginSceneLoad(const std::string& aScenePath);
	void MarkAsyncLoadFinished();
	void FinishSceneLoadAndPrint();

	void RecordPhase(const std::string& aName, double aMilliseconds);
	void RecordLevelDataStats(std::size_t aFileCount, std::uintmax_t aByteCount);
	void RecordSceneCacheStatus(const std::string& aStatus);
	void RecordObjectDefinitionStats(std::size_t aFileCount, std::uintmax_t aByteCount);
	void RecordObjectCount(std::size_t anObjectCount);
	void RecordTag(const std::string& aTag);
	void RecordModelPath(const std::string& aPath);
	void RecordTexturePath(const std::string& aPath);
	void RecordAnimationPath(const std::string& aPath);
	void RecordModelLoad(const std::string& aPath, double aMilliseconds, bool aCacheHit);
	void RecordAnimationLoad(const std::string& aPath, double aMilliseconds, bool aCacheHit);
	void RecordGameObjectInit(const std::string& aName, const std::string& aTag, double aMilliseconds);
	void RecordScriptStart(const std::string& aComponentName, double aMilliseconds);

private:
	struct PhaseTiming
	{
		double totalMilliseconds = 0.0;
		std::size_t count = 0;
	};

	struct AssetTiming
	{
		std::string path;
		double milliseconds = 0.0;
		bool cacheHit = false;
	};

	struct ObjectInitTiming
	{
		std::string name;
		std::string tag;
		double milliseconds = 0.0;
	};

	struct Report
	{
		bool active = false;
		std::string scenePath;
		std::chrono::steady_clock::time_point startTime;
		double asyncMilliseconds = 0.0;
		std::size_t levelDataFileCount = 0;
		std::uintmax_t levelDataByteCount = 0;
		std::string sceneCacheStatus;
		std::size_t objectDefinitionFileCount = 0;
		std::uintmax_t objectDefinitionByteCount = 0;
		std::size_t objectCount = 0;
		std::unordered_map<std::string, PhaseTiming> phases;
		std::unordered_map<std::string, std::size_t> tagCounts;
		std::unordered_set<std::string> modelPaths;
		std::unordered_set<std::string> texturePaths;
		std::unordered_set<std::string> animationPaths;
		std::vector<AssetTiming> modelLoads;
		std::vector<AssetTiming> animationLoads;
		std::vector<ObjectInitTiming> objectInits;
		std::vector<AssetTiming> scriptStarts;
	};

	LoadingProfiler() = default;

	void PrintReportLocked(const Report& aReport, double aTotalMilliseconds) const;

	mutable std::mutex myMutex;
	Report myReport;
};
}
