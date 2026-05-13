#include <stdafx.h>
#include "SceneSerialize.h"
#include "Scene.h"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <thread>

#include <tge/debug/LoadingProfiler.h>
#include <tge/stringRegistry/StringRegistry.h>
#include <tge/model/ModelInstance.h>
#include <tge/model/ModelFactory.h>
#include <tge/settings/settings.h>
#include <tge/script/jsondata.h>

#include <tge/scene/SceneObjectDefinitionManager.h>

#include <nlohmann/json.hpp>
using namespace nlohmann;
using namespace Tga;

namespace
{
	struct ParsedSceneProperty
	{
		std::string name;
		std::string type;
		nlohmann::json value;
	};

	struct ParsedSceneObject
	{
		std::string uuid;
		std::string name = "unknown";
		std::string path;
		std::string objectDefinition;
		bool hasTransform = false;
		std::array<float, 16> transform = {};
		bool hasTranslation = false;
		std::array<float, 3> translation = {};
		bool hasRotation = false;
		std::array<float, 3> rotation = {};
		bool hasScale = false;
		std::array<float, 3> scale = {};
		std::vector<ParsedSceneProperty> properties;
	};

	struct SourceSceneCacheStats
	{
		std::int64_t sourceTgsWriteTime = 0;
		std::size_t levelDataFileCount = 0;
		std::uintmax_t levelDataByteCount = 0;
		std::int64_t newestLevelDataWriteTime = 0;
	};

	enum class PackedSceneCacheReadResult
	{
		Hit,
		Missing,
		Stale,
		Corrupt
	};

	constexpr int kPackedSceneCacheVersion = 1;
	constexpr const char* kPackedSceneCacheFolder = ".scene_cache";
	constexpr const char* kPackedSceneCacheExtension = ".tgscache";

	std::int64_t GetWriteTimeStamp(const fs::path& aPath)
	{
		std::error_code error;
		const fs::file_time_type writeTime = fs::last_write_time(aPath, error);
		if (error)
		{
			return 0;
		}

		return static_cast<std::int64_t>(writeTime.time_since_epoch().count());
	}

	fs::path GetPackedSceneCachePath(const fs::path& aResolvedTgsPath)
	{
		fs::path cachePath = aResolvedTgsPath.parent_path() / kPackedSceneCacheFolder;
		cachePath /= aResolvedTgsPath.stem();
		cachePath.replace_extension(kPackedSceneCacheExtension);
		return cachePath;
	}

	void CollectSceneObjectPathsAndStats(
		const fs::path& aResolvedTgsPath,
		const fs::path& aDataPath,
		std::vector<fs::path>& outSceneObjectPaths,
		SourceSceneCacheStats& outStats)
	{
		LoadingProfiler::Scope scope("Tga::LoadSceneCacheCheck");

		outStats = {};
		outStats.sourceTgsWriteTime = GetWriteTimeStamp(aResolvedTgsPath);
		outSceneObjectPaths.clear();

		for (const fs::directory_entry sceneItem : fs::directory_iterator(aDataPath))
		{
			if (sceneItem.path().filename().has_extension())
			{
				continue;
			}

			++outStats.levelDataFileCount;

			std::error_code fileSizeError;
			const std::uintmax_t fileSize = fs::file_size(sceneItem.path(), fileSizeError);
			if (!fileSizeError)
			{
				outStats.levelDataByteCount += fileSize;
			}

			const std::int64_t writeTime = GetWriteTimeStamp(sceneItem.path());
			outStats.newestLevelDataWriteTime = (std::max)(outStats.newestLevelDataWriteTime, writeTime);
			outSceneObjectPaths.push_back(sceneItem.path());
		}
	}

	ParsedSceneObject ParseSceneObjectJson(const std::string& aUuid, const json& anItem)
	{
		ParsedSceneObject result;
		result.uuid = aUuid;

		result.name = anItem.value("name", "unknown");
		result.path = anItem.value("path", "");

		if (anItem.contains("transform"))
		{
			result.hasTransform = true;
			result.transform = anItem["transform"].get<std::array<float, 16>>();
		}

		if (anItem.contains("translation"))
		{
			result.hasTranslation = true;
			result.translation = anItem["translation"].get<std::array<float, 3>>();
		}

		if (anItem.contains("rotation"))
		{
			result.hasRotation = true;
			result.rotation = anItem["rotation"].get<std::array<float, 3>>();
		}

		if (anItem.contains("scale"))
		{
			result.hasScale = true;
			result.scale = anItem["scale"].get<std::array<float, 3>>();
		}

		if (anItem.contains("object-definition"))
		{
			result.objectDefinition = anItem["object-definition"].get<std::string>();
		}

		if (anItem.contains("properties"))
		{
			result.properties.reserve(anItem["properties"].size());
			for (const auto& propertyJson : anItem["properties"])
			{
				ParsedSceneProperty property;
				property.name = propertyJson["name"].get<std::string>();
				property.type = propertyJson["type"].get<std::string>();
				property.value = propertyJson["value"];
				result.properties.push_back(std::move(property));
			}
		}

		return result;
	}

	ParsedSceneObject ParseSceneObjectFile(const fs::path& aPath)
	{
		std::ifstream objIn(aPath, std::ios::in);
		assert(objIn);
		json item;
		objIn >> item;

		return ParseSceneObjectJson(aPath.stem().string(), item);
	}

	json WriteSceneObjectJson(const ParsedSceneObject& anObject)
	{
		json objectJson = {
			{ "uuid", anObject.uuid },
			{ "name", anObject.name },
			{ "path", anObject.path },
			{ "object-definition", anObject.objectDefinition }
		};

		if (anObject.hasTransform)
		{
			objectJson["transform"] = anObject.transform;
		}
		if (anObject.hasTranslation)
		{
			objectJson["translation"] = anObject.translation;
		}
		if (anObject.hasRotation)
		{
			objectJson["rotation"] = anObject.rotation;
		}
		if (anObject.hasScale)
		{
			objectJson["scale"] = anObject.scale;
		}

		for (const ParsedSceneProperty& property : anObject.properties)
		{
			objectJson["properties"].push_back({
				{ "name", property.name },
				{ "type", property.type },
				{ "value", property.value }
			});
		}

		return objectJson;
	}

	void CreateSceneObjectFromJson(const std::string& aUuid, const json& anObjectJson, Scene& scene)
	{
		SceneObject object;
		object.SetName(anObjectJson.value("name", "unknown").c_str());
		object.SetPath(nullptr, StringRegistry::RegisterOrGetString(anObjectJson.value("path", "").c_str()));

		if (anObjectJson.contains("transform"))
		{
			std::array<float, 16> transform = anObjectJson["transform"].get<std::array<float, 16>>();
			object.SetTransform(Matrix4x4f(transform.data()));
		}

		if (anObjectJson.contains("translation"))
		{
			const std::array<float, 3> translation = anObjectJson["translation"].get<std::array<float, 3>>();
			object.GetTRS().translation = { translation[0], translation[1], translation[2] };
		}

		if (anObjectJson.contains("rotation"))
		{
			const std::array<float, 3> rotation = anObjectJson["rotation"].get<std::array<float, 3>>();
			object.GetTRS().rotation = { rotation[0], rotation[1], rotation[2] };
		}

		if (anObjectJson.contains("scale"))
		{
			const std::array<float, 3> scale = anObjectJson["scale"].get<std::array<float, 3>>();
			object.GetTRS().scale = { scale[0], scale[1], scale[2] };
		}

		const std::string objectDefinition = anObjectJson.value("object-definition", "");
		if (!objectDefinition.empty())
		{
			object.SetSceneObjectDefintionName(StringRegistry::RegisterOrGetString(objectDefinition));
		}

		if (anObjectJson.contains("properties"))
		{
			std::vector<SceneProperty>& properties = object.EditPropertyOverrides();
			properties.reserve(anObjectJson["properties"].size());
			for (const auto& propertyJson : anObjectJson["properties"])
			{
				SceneProperty propertyDefinition = {};
				propertyDefinition.name = StringRegistry::RegisterOrGetString(propertyJson["name"].get<std::string>());
				propertyDefinition.type = PropertyTypeRegistry::GetPropertyType(StringRegistry::RegisterOrGetString(propertyJson["type"].get<std::string>()));

				JsonData valueJson = { propertyJson["value"] };
				propertyDefinition.value = Property::CreateFromJson(propertyDefinition.type, valueJson);

				properties.push_back(propertyDefinition);
			}
		}

		scene.CreateSceneObject(aUuid.c_str(), object);
	}

	bool CacheMetadataMatches(
		const json& aCacheJson,
		const std::string& aScenePath,
		const SourceSceneCacheStats& someStats)
	{
		return aCacheJson.value("version", 0) == kPackedSceneCacheVersion
			&& aCacheJson.value("source-scene", "") == aScenePath
			&& aCacheJson.value("source-tgs-write-time", std::int64_t{ 0 }) == someStats.sourceTgsWriteTime
			&& aCacheJson.value("leveldata-file-count", std::uint64_t{ 0 }) == static_cast<std::uint64_t>(someStats.levelDataFileCount)
			&& aCacheJson.value("leveldata-total-bytes", std::uint64_t{ 0 }) == static_cast<std::uint64_t>(someStats.levelDataByteCount)
			&& aCacheJson.value("newest-leveldata-write-time", std::int64_t{ 0 }) == someStats.newestLevelDataWriteTime;
	}

	PackedSceneCacheReadResult TryLoadPackedSceneCache(
		const std::string& aScenePath,
		const fs::path& aCachePath,
		const SourceSceneCacheStats& someStats,
		Scene& scene)
	{
		if (!fs::exists(aCachePath))
		{
			return PackedSceneCacheReadResult::Missing;
		}

		try
		{
			LoadingProfiler::Scope scope("Tga::LoadScenePackedCache");

			std::ifstream cacheInput(aCachePath, std::ios::in);
			if (!cacheInput)
			{
				return PackedSceneCacheReadResult::Stale;
			}

			json cacheJson;
			cacheInput >> cacheJson;

			if (!CacheMetadataMatches(cacheJson, aScenePath, someStats))
			{
				return PackedSceneCacheReadResult::Stale;
			}

			const json& objectsJson = cacheJson["objects"];
			{
				LoadingProfiler::Scope buildScope("Tga::LoadScenePackedCacheBuildSceneObjects");
				for (const auto& objectJson : objectsJson)
				{
					CreateSceneObjectFromJson(objectJson["uuid"].get<std::string>(), objectJson, scene);
				}
			}

			return PackedSceneCacheReadResult::Hit;
		}
		catch (const std::exception& exception)
		{
#ifndef _RETAIL
			std::cout << "Scene cache read failed for " << aCachePath.string() << ": " << exception.what() << "\n";
#else
			(void)exception;
#endif
			return PackedSceneCacheReadResult::Corrupt;
		}
		catch (...)
		{
#ifndef _RETAIL
			std::cout << "Scene cache read failed for " << aCachePath.string() << "\n";
#endif
			return PackedSceneCacheReadResult::Corrupt;
		}
	}

	bool WritePackedSceneCache(
		const std::string& aScenePath,
		const fs::path& aCachePath,
		const SourceSceneCacheStats& someStats,
		const std::vector<ParsedSceneObject>& someParsedObjects)
	{
		try
		{
			LoadingProfiler::Scope scope("Tga::WriteScenePackedCache");

			fs::create_directories(aCachePath.parent_path());

			json cacheJson = {
				{ "version", kPackedSceneCacheVersion },
				{ "source-scene", aScenePath },
				{ "source-tgs-write-time", someStats.sourceTgsWriteTime },
				{ "leveldata-file-count", static_cast<std::uint64_t>(someStats.levelDataFileCount) },
				{ "leveldata-total-bytes", static_cast<std::uint64_t>(someStats.levelDataByteCount) },
				{ "newest-leveldata-write-time", someStats.newestLevelDataWriteTime }
			};

			cacheJson["objects"] = json::array();
			for (const ParsedSceneObject& parsedObject : someParsedObjects)
			{
				cacheJson["objects"].push_back(WriteSceneObjectJson(parsedObject));
			}

			std::ofstream cacheOutput(aCachePath, std::ios::out | std::ios::trunc);
			if (!cacheOutput)
			{
				return false;
			}

			cacheOutput << cacheJson.dump();
			return true;
		}
		catch (const std::exception& exception)
		{
#ifndef _RETAIL
			std::cout << "Scene cache write failed for " << aCachePath.string() << ": " << exception.what() << "\n";
#else
			(void)exception;
#endif
			return false;
		}
		catch (...)
		{
#ifndef _RETAIL
			std::cout << "Scene cache write failed for " << aCachePath.string() << "\n";
#endif
			return false;
		}
	}

	std::vector<ParsedSceneObject> LoadSceneObjectFilesParallel(const std::vector<fs::path>& somePaths)
	{
		if (somePaths.empty())
		{
			return {};
		}

		const unsigned int hardwareThreads = (std::max)(1u, std::thread::hardware_concurrency());
		const std::size_t workerCount = (std::min<std::size_t>)(hardwareThreads, somePaths.size());
		const std::size_t chunkSize = (somePaths.size() + workerCount - 1) / workerCount;

		std::vector<std::future<std::vector<ParsedSceneObject>>> futures;
		futures.reserve(workerCount);

		for (std::size_t workerIndex = 0; workerIndex < workerCount; ++workerIndex)
		{
			const std::size_t beginIndex = workerIndex * chunkSize;
			if (beginIndex >= somePaths.size())
			{
				break;
			}

			const std::size_t endIndex = (std::min)(beginIndex + chunkSize, somePaths.size());
			futures.push_back(std::async(std::launch::async, [&somePaths, beginIndex, endIndex]()
				{
					std::vector<ParsedSceneObject> parsedObjects;
					parsedObjects.reserve(endIndex - beginIndex);
					for (std::size_t index = beginIndex; index < endIndex; ++index)
					{
						parsedObjects.push_back(ParseSceneObjectFile(somePaths[index]));
					}
					return parsedObjects;
				}));
		}

		std::vector<ParsedSceneObject> result;
		result.reserve(somePaths.size());
		for (auto& future : futures)
		{
			std::vector<ParsedSceneObject> chunk = future.get();
			std::move(chunk.begin(), chunk.end(), std::back_inserter(result));
		}

		return result;
	}

	void CreateSceneObjectsFromParsedObjects(const std::vector<ParsedSceneObject>& someParsedObjects, Scene& scene)
	{
		LoadingProfiler::Scope scope("Tga::CreateSceneObjectsFromParsedObjects");
		for (const ParsedSceneObject& parsedObject : someParsedObjects)
		{
			SceneObject object;
			object.SetName(parsedObject.name.c_str());
			object.SetPath(nullptr, StringRegistry::RegisterOrGetString(parsedObject.path.c_str()));

			if (parsedObject.hasTransform)
			{
				std::array<float, 16> transform = parsedObject.transform;
				object.SetTransform(Matrix4x4f(transform.data()));
			}

			if (parsedObject.hasTranslation)
			{
				const std::array<float, 3>& translation = parsedObject.translation;
				object.GetTRS().translation = { translation[0], translation[1], translation[2] };
			}

			if (parsedObject.hasRotation)
			{
				const std::array<float, 3>& rotation = parsedObject.rotation;
				object.GetTRS().rotation = { rotation[0], rotation[1], rotation[2] };
			}

			if (parsedObject.hasScale)
			{
				const std::array<float, 3>& scale = parsedObject.scale;
				object.GetTRS().scale = { scale[0], scale[1], scale[2] };
			}

			if (!parsedObject.objectDefinition.empty())
			{
				object.SetSceneObjectDefintionName(StringRegistry::RegisterOrGetString(parsedObject.objectDefinition));
			}

			std::vector<SceneProperty>& properties = object.EditPropertyOverrides();
			properties.reserve(parsedObject.properties.size());
			for (const ParsedSceneProperty& parsedProperty : parsedObject.properties)
			{
				SceneProperty propertyDefinition = {};
				propertyDefinition.name = StringRegistry::RegisterOrGetString(parsedProperty.name);
				propertyDefinition.type = PropertyTypeRegistry::GetPropertyType(StringRegistry::RegisterOrGetString(parsedProperty.type));

				JsonData valueJson = { parsedProperty.value };
				propertyDefinition.value = Property::CreateFromJson(propertyDefinition.type, valueJson);

				properties.push_back(propertyDefinition);
			}
			scene.CreateSceneObject(parsedObject.uuid.c_str(), object);
		}
	}
}


extern void Tga::SaveScene(const Scene& scene, void (*afileChangedCallback)(SceneFileChangeType, const char*))
{
	std::filesystem::path path = (Tga::Settings::GameAssetRoot() / scene.GetPath());
	std::filesystem::path levelDataPath = path;

	std::filesystem::path levelDataRelativePath = scene.GetPath();

	levelDataPath.replace_extension(".leveldata");
	levelDataRelativePath.replace_extension(".leveldata");

	if (path.empty())
		return;
	if (path.extension().empty())
	{
		path = path.replace_extension(".tgs");
	}

	if (std::filesystem::is_directory(path.parent_path()) == false)
	{
		fs::permissions(path.parent_path().parent_path(), fs::perms::all);
		std::filesystem::create_directories(path.parent_path());
	}

	if (std::filesystem::is_directory(levelDataPath) == false)
	{
		fs::permissions(levelDataPath.parent_path(), fs::perms::all);
		std::filesystem::create_directories(levelDataPath);
	}

	std::set<std::string> remainingFiles;

	for (const auto& entry : fs::directory_iterator(levelDataPath))
	{
		std::string filename = entry.path().stem().string();
		for (auto& c : filename)
		{
			c = (char)tolower(c);
		}

		remainingFiles.insert(filename);
	}


	for (const auto& object : scene.GetSceneObjects()) 
	{
		const char* uuid = UUIDManager::GetUUIDStringFromID(object.first);

		json transform;
		for (size_t i = 0; i < 16; i++) {
			transform.push_back(object.second->GetTransform().GetDataPtr()[i]);
		}

		Vector3f pos = object.second->GetPosition();
		Vector3f euler = object.second->GetEuler();
		Vector3f scale = object.second->GetScale();

		json jsonobj = {
			{ "name", object.second->GetName()},
			{ "path", object.second->GetPath().GetString()},
			{ "translation", {pos.x, pos.y, pos.z} },
			{ "rotation", {euler.x, euler.y, euler.z} },
			{ "scale", {scale.x, scale.y, scale.z} },
			{ "object-definition", object.second->GetSceneObjectDefinitionName().GetString() },
		};

		for (const SceneProperty& propertyDefinition : object.second->GetPropertyOverrides())
		{
			JsonData value = {};

			if (propertyDefinition.value.HasValue())
				propertyDefinition.value.WriteToJsonWithoutType(value);

			json propertyJson = {
				{ "name", propertyDefinition.name.GetString() },
				{ "type", propertyDefinition.type->GetName().GetString()},
				{ "value", value.json},
			};

			jsonobj["properties"].push_back(propertyJson);
		}
	
		std::filesystem::path objpath = levelDataPath / uuid;

		bool existed = false;
		{
			auto it = remainingFiles.find(uuid);
			if (it != remainingFiles.end())
			{
				fs::permissions(objpath, fs::perms::all);

				remainingFiles.erase(uuid);

				existed = true;
			}
		}

		std::ofstream objout(objpath.string(), std::ios::out | std::ios::trunc);
		objout << jsonobj.dump(2);

		if (afileChangedCallback != nullptr)
		{
			std::string objpathRelative = (levelDataRelativePath / uuid).string();
			std::replace(objpathRelative.begin(), objpathRelative.end(), '\\', '/');
			afileChangedCallback(existed ? SceneFileChangeType::Modify : SceneFileChangeType::Add, objpathRelative.c_str());
		}

		UUIDManager::DeallocateUUIDString(uuid);
	}

	for (const std::string& uuid : remainingFiles)
	{
		std::filesystem::path objpath = levelDataPath / uuid;

		std::filesystem::remove(objpath);

		if (afileChangedCallback != nullptr)
		{
			std::string objpathRelative = (levelDataRelativePath / uuid).string();
			std::replace(objpathRelative.begin(), objpathRelative.end(), '\\', '/');
			afileChangedCallback(SceneFileChangeType::Delete, objpathRelative.c_str());
		}
	}

	json scenejson;


	std::ofstream fout(path, std::ios::trunc);
	fs::permissions(path, fs::perms::all);
	fout << scenejson.dump(2);
}

extern bool Tga::LoadScene(const char* filepath, Scene& scene)
{
	LoadingProfiler::Scope scope("Tga::LoadScene");

	std::filesystem::path resolvedTgsPath = Tga::Settings::GameAssetRoot() / filepath;

	scene.SetPath(filepath);
	std::string stem = resolvedTgsPath.stem().string();
	scene.SetName(stem.c_str());
	scene.ClearScene();

	std::filesystem::path dataPath = resolvedTgsPath;
	dataPath.replace_extension(".leveldata");

	if (!fs::exists(dataPath)) // empty scenes can miss the leveldata folder
		return true;

	std::vector<fs::path> sceneObjectPaths;
	SourceSceneCacheStats sourceStats;
	CollectSceneObjectPathsAndStats(resolvedTgsPath, dataPath, sceneObjectPaths, sourceStats);
	LoadingProfiler::GetInstance().RecordLevelDataStats(sourceStats.levelDataFileCount, sourceStats.levelDataByteCount);

	const fs::path cachePath = GetPackedSceneCachePath(resolvedTgsPath);
	std::vector<ParsedSceneObject> parsedObjects;
	const PackedSceneCacheReadResult cacheReadResult = TryLoadPackedSceneCache(filepath, cachePath, sourceStats, scene);
	if (cacheReadResult == PackedSceneCacheReadResult::Hit)
	{
		LoadingProfiler::GetInstance().RecordSceneCacheStatus("cache-hit");
		return true;
	}

	scene.ClearScene();

	{
		LoadingProfiler::Scope fallbackScope("Tga::LoadSceneLevelDataFallback");
		parsedObjects = LoadSceneObjectFilesParallel(sceneObjectPaths);
	}

	const bool wroteCache = WritePackedSceneCache(filepath, cachePath, sourceStats, parsedObjects);
	if (!wroteCache)
	{
		LoadingProfiler::GetInstance().RecordSceneCacheStatus("cache-write-failed");
	}
	else if (cacheReadResult == PackedSceneCacheReadResult::Missing)
	{
		LoadingProfiler::GetInstance().RecordSceneCacheStatus("cache-miss");
	}
	else if (cacheReadResult == PackedSceneCacheReadResult::Corrupt)
	{
		LoadingProfiler::GetInstance().RecordSceneCacheStatus("cache-corrupt");
	}
	else
	{
		LoadingProfiler::GetInstance().RecordSceneCacheStatus("cache-stale");
	}

	CreateSceneObjectsFromParsedObjects(parsedObjects, scene);
	return true;
}
