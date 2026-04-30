#pragma once

#include <filesystem>
#include <unordered_map>
#include <memory>
#include <string_view>

#include <tge/stringRegistry/StringRegistry.h>
#include "tge/UI/CanvasObjectDefinition.h"

namespace Tga
{
	class CanvasObjectDefinitionManager
	{
	public:
		CanvasObjectDefinitionManager();
		void Init(std::string_view aProjectPath);

		CanvasObjectDefinition* CreateOrGet(const std::filesystem::path& aPath);
		CanvasObjectDefinition* Get(StringId name);

	private:
		std::unordered_map<StringId, std::unique_ptr<CanvasObjectDefinition>> myCanvasObjectDefinitions;
	};
}
