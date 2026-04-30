#include <stdafx.h>

#include "CanvasObjectDefinitionManager.h"
#include "CanvasObjectDefinition.h"

#include <fstream>

using namespace Tga;

CanvasObjectDefinitionManager::CanvasObjectDefinitionManager() {}

void CanvasObjectDefinitionManager::Init(std::string_view aProjectPath)
{
	for (const auto& entry : fs::recursive_directory_iterator(aProjectPath))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".canvas")
		{
			std::unique_ptr<CanvasObjectDefinition> definition = std::make_unique<CanvasObjectDefinition>();

			std::string path = fs::relative(entry.path(), Tga::Settings::GameAssetRoot()).string();
			definition->Load(path.c_str());

			if (myCanvasObjectDefinitions.find(definition->GetName()) != myCanvasObjectDefinitions.end())
			{
				std::cout << "Multiple canvas files exist with same name. That is not allowed: " << definition->GetName().GetString() << "\n";
			}
			else
			{
				myCanvasObjectDefinitions[definition->GetName()] = std::move(definition);
			}
		}
	}
}

CanvasObjectDefinition* CanvasObjectDefinitionManager::CreateOrGet(const fs::path& aPath)
{
	std::unique_ptr<CanvasObjectDefinition> canvasDefinition = std::make_unique<CanvasObjectDefinition>();

	std::string name = aPath.stem().string();
	StringId nameId = StringRegistry::RegisterOrGetString(name.c_str());

	auto it = myCanvasObjectDefinitions.find(nameId);
	if (it != myCanvasObjectDefinitions.end())
		return it->second.get();

	std::string pathString = aPath.string().c_str();

	canvasDefinition->SetName(nameId);
	canvasDefinition->SetPath(pathString.c_str());
	canvasDefinition->Save();

	myCanvasObjectDefinitions[nameId] = std::move(canvasDefinition);
	return myCanvasObjectDefinitions[nameId].get();
}

CanvasObjectDefinition* CanvasObjectDefinitionManager::Get(StringId name)
{
	auto it = myCanvasObjectDefinitions.find(name);
	if (it == myCanvasObjectDefinitions.end())
		return nullptr;

	return it->second.get();
}