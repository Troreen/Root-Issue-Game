#include "EditorSceneManager.h"

#include <fstream>
#include <tge/settings/settings.h>
#include <tge/scene/SceneSerialize.h>

using namespace Tga;

EditorSceneManager::EditorSceneManager() {}

Scene* EditorSceneManager::Get(const std::filesystem::path& aPath)
{
	auto it = myScenes.find(aPath);
	if (it == myScenes.end())
	{
		std::filesystem::path resolvedTgsPath = Tga::Settings::GameAssetRoot() / aPath;
		if (!fs::exists(resolvedTgsPath))
			return nullptr;

		std::unique_ptr<Scene> scene = std::make_unique<Scene>();

		std::string pathString = aPath.string();
		LoadScene(pathString.c_str(), *scene);

		myScenes[aPath] = std::move(scene);
	}

	return myScenes[aPath].get();
}