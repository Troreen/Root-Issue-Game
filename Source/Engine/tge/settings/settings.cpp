#include <stdafx.h>
#include "settings.h"

#include <filesystem>

#include <nlohmann/json.hpp>
#include <fstream>

#include <tge/util/StringCast.h>
#include <tge/stringRegistry/StringRegistry.h>

namespace fs = std::filesystem;

namespace Tga
{
	namespace Settings
	{
		static fs::path engine_assets_path;
		static fs::path game_assets_path;
		static fs::path cooked_assets_path;

		static fs::path executable_folder_path;

		static EngineConfiguration window_params;
	}
}


const fs::path& Tga::Settings::EngineAssetRoot()
{
	return Tga::Settings::engine_assets_path;
}
const fs::path& Tga::Settings::GameAssetRoot()
{
	return Tga::Settings::game_assets_path;
}
const fs::path& Tga::Settings::CookedAssetRoot()
{
	return Tga::Settings::cooked_assets_path;
}

Tga::EngineConfiguration& Tga::Settings::GetEngineConfiguration()
{
	return window_params;
}

std::string Tga::Settings::ResolveAssetPath(const std::string_view& anAsset)
{
	if (fs::exists(executable_folder_path / std::filesystem::path("data/") / anAsset))
	{
		return (executable_folder_path / std::filesystem::path("data/") / anAsset).string();
	}
	else if (!game_assets_path.empty() && fs::exists(game_assets_path / anAsset))
	{
		return (game_assets_path / anAsset).string();
	}
	else if (!engine_assets_path.empty() && fs::exists(engine_assets_path / anAsset))
	{
		return (engine_assets_path / anAsset).string();
	}

	return "";
}

std::string Tga::Settings::ResolveEngineAssetPath(const std::string_view& anAsset)
{
	if (fs::exists(executable_folder_path / std::filesystem::path("data/") / anAsset))
	{
		return (executable_folder_path / std::filesystem::path("data/") / anAsset).string();
	}
	else if (!engine_assets_path.empty() && fs::exists(engine_assets_path / anAsset))
	{
		return (engine_assets_path / anAsset).string();
	}
	return "";
}

std::string Tga::Settings::ResolveGameAssetPath(const std::string_view& anAsset)
{
	if (fs::exists(executable_folder_path / std::filesystem::path("data/") / anAsset))
	{
		return (executable_folder_path / std::filesystem::path("data/") / anAsset).string();
	}
	else if (!game_assets_path.empty() && fs::exists(game_assets_path / anAsset))
	{
		return (game_assets_path / anAsset).string();
	}
	return "";
}

std::string Tga::Settings::ResolveCookedAssetPath(const std::string_view& anAsset)
{
	if (fs::exists(executable_folder_path / std::filesystem::path("CookedAssets/") / anAsset))
	{
		return (executable_folder_path / std::filesystem::path("CookedAssets/") / anAsset).string();
	}
	else if (!cooked_assets_path.empty() && fs::exists(cooked_assets_path / anAsset))
	{
		return (cooked_assets_path / anAsset).string();
	}
	return "";
}

bool Tga::LoadSettings(const std::string& aProjectName)
{
	using namespace Settings;
	using namespace nlohmann;

	WCHAR executablePathWString[MAX_PATH]{ 0 };
	// Game settings
	if (!GetModuleFileName(NULL, executablePathWString, sizeof(executablePathWString)))
	{
		assert(false && "GetModuleFileName failed in Tga::LoadSettings");
		return false;
	}

	std::filesystem::path executablePath(executablePathWString);
	executable_folder_path = executablePath.parent_path();

	std::string executableFolder = executable_folder_path.string();
	std::string settingsFolder = executableFolder + "\\settings\\";
	std::string filename = (aProjectName.find(".") == std::string::npos) ? (aProjectName + ".json") : aProjectName;
	std::string settingsFilepath = settingsFolder + filename;
	std::ifstream game_ifs(settingsFilepath.c_str());

	if (!game_ifs)
	{
		assert(false && "Could not open project settings file in Tga::LoadSettings");
		return false;
	}

	nlohmann::json game_settings;
	game_ifs >> game_settings;
	game_ifs.close();

	if (game_settings.contains("assets_path"))
	{
		engine_assets_path = (executable_folder_path / game_settings["assets_path"]["engine"]).lexically_normal();
		game_assets_path = (executable_folder_path / game_settings["assets_path"]["game"]).lexically_normal();

		// allow not specifying cooked path since it was added recently and the default value will work fine.
		if (game_settings["assets_path"].contains("cooked"))
			cooked_assets_path = (executable_folder_path / game_settings["assets_path"]["cooked"]).lexically_normal();
	}

	//////////////////////////////////////
	// Window Title
	{
		std::string app = game_settings["window_settings"]["title"];
		if (!app.empty()) {
			Settings::window_params.myApplicationName = string_cast<std::wstring>(app);
		}
	}
	/////////////////////////////////////
	// Clear Color
	{
		auto& app = game_settings["window_settings"]["clear_color"];
		if (!app.is_null()) {
			Settings::window_params.myClearColor = Tga::Color(app["r"], app["g"], app["b"], app["a"]);
		}
	}
	/////////////////////////////////////
	// Window Width & Height
	{
		auto& window = game_settings["window_settings"]["window_size"];
		auto& render = game_settings["window_settings"]["render_size"];

		if (window.is_null() == false) {
			Settings::window_params.myWindowSize.x = window["w"];
			Settings::window_params.myWindowSize.y = window["h"];
			if (render.is_null()) {
				Settings::window_params.myRenderSize.x = window["w"];
				Settings::window_params.myRenderSize.y = window["h"];
			}
			else {
				Settings::window_params.myRenderSize.x = render["w"];
				Settings::window_params.myRenderSize.y = render["h"];
			}
		}
		else if (render.is_null() == false) {
			Settings::window_params.myRenderSize.x = render["w"];
			Settings::window_params.myRenderSize.y = render["h"];
			Settings::window_params.myWindowSize.x = render["w"];
			Settings::window_params.myWindowSize.y = render["h"];
		}
	}
	//////////////////////////////////////
	// VSync
	{
		auto& app = game_settings["enable_vsync"];
		if (!app.is_null()) {
			Settings::window_params.myEnableVSync = app;
		}
	}
	/////////////////////////////////////
	// Start in fullscreen / Maximized
	{
		auto& app = game_settings["window_settings"]["start_in_fullscreen"];
		if (!app.is_null()) {
			Settings::window_params.myStartInFullScreen = app;
		}
	}
	{
		auto& app = game_settings["window_settings"]["start_maximized"];
		if (!app.is_null()) {
			Settings::window_params.myStartMaximized = app;
		}
	}

	/////////////////////////////////////
	// Window setting (overlapped/borderless)
	{
		auto& app = game_settings["window_settings"]["borderless"];
		if (!app.is_null()) {
			Settings::window_params.myBorderless = app;
		}
	}

	/////////////////////////////////////
	// Debug systems
	{
		auto& app = game_settings["debug_features"];
		DebugFeature dbg = static_cast<DebugFeature>(0);

		for (std::string flag : app) {
			if (flag == "All") { dbg = DebugFeature::All; break; }
			if (flag == "None") { dbg = DebugFeature::None; break; }
			if (flag == "Cpu") { dbg = dbg | DebugFeature::Cpu; continue; }
			if (flag == "Drawcalls") { dbg = dbg | DebugFeature::Drawcalls; continue; }
			if (flag == "Filewatcher") { dbg = dbg | DebugFeature::Filewatcher; continue; }
			if (flag == "Fps") { dbg = dbg | DebugFeature::Fps; continue; }
			if (flag == "FpsGraph") { dbg = dbg | DebugFeature::FpsGraph; continue; }
			if (flag == "Mem") { dbg = dbg | DebugFeature::Mem; continue; }
			if (flag == "MemoryTrackingAllAllocations") { dbg = dbg | DebugFeature::MemoryTrackingAllAllocations; continue; }
			if (flag == "MemoryTrackingStackTraces") { dbg = dbg | DebugFeature::MemoryTrackingStackTraces; continue; }
			if (flag == "OptimizeWarnings") { dbg = dbg | DebugFeature::OptimizeWarnings; continue; }
		}
		//if (dbg > static_cast<DebugFeature>(0))
		{
			Settings::window_params.myActivateDebugSystems = dbg;
		}
	}

	/////////////////////////////////////
	// Multisampling Quality
	{
		auto& app = game_settings["multisampling"];

		for (std::string flag : app) {
			if (flag == "Off") {
				Settings::window_params.myPreferedMultiSamplingQuality = MultiSamplingQuality::Off;
			}
			if (flag == "Low") {
				Settings::window_params.myPreferedMultiSamplingQuality = MultiSamplingQuality::Low;
			}
			if (flag == "Medium") {
				Settings::window_params.myPreferedMultiSamplingQuality = MultiSamplingQuality::Medium;
			}
			if (flag == "High") {
				Settings::window_params.myPreferedMultiSamplingQuality = MultiSamplingQuality::High;
			}
		}
	}

	return true;
}
