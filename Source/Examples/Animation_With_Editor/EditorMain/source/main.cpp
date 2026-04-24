#include <GoEditor.h>
#include <TGAFBXImporter/Source/Importer.h>

int main(const int /*argc*/, const char* /*argc*/[])
{
	TGA::FBX::Importer::SetShouldBakeSkeletonRootTransforms(true);

	EditorConfiguration configuration = {};
	configuration.enableVisualScripts = true;
	configuration.debugExeName = "AnimationGameMain_Debug.exe";
	configuration.releaseExeName = "AnimationGameMain_Release.exe";
	configuration.debugExePath = L"..\\Bin\\AnimationGameMain_Debug.exe";
	configuration.releaseExePath = L"..\\Bin\\AnimationGameMain_Release.exe";

	GoEditor(TGE_PROJECT_SETTINGS_FILE, configuration);
	return 0;
}
