#pragma once

#include <tge/engine.h>
#include <filesystem>
namespace fs = std::filesystem;

namespace Tga
{
	extern bool LoadSettings(const std::string& aProjectName);
	
	namespace Settings
	{
		extern const fs::path& EngineAssetRoot();
		extern const fs::path& GameAssetRoot();
		extern const fs::path& CookedAssetRoot();

		extern EngineConfiguration& GetEngineConfiguration();

		extern std::string ResolveAssetPath(const std::string_view& anAsset);
		extern std::string ResolveEngineAssetPath(const std::string_view& anAsset);
		extern std::string ResolveGameAssetPath(const std::string_view& anAsset);
		extern std::string ResolveCookedAssetPath(const std::string_view& anAsset);
	}
}
