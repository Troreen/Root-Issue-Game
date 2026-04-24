#include <GoEditor.h>
#include <TGAFBXImporter/Source/Importer.h>
#include "Nodes.h"

int main(const int /*argc*/, const char* /*argc*/[])
{
	RegisterGameNodes();


	EditorConfiguration configuration = {};
	configuration.enableVisualScripts = true;
	configuration.debugExeName = "ScriptGameMain_Debug.exe";
	configuration.releaseExeName = "ScriptGameMain_Release.exe";
	configuration.debugExePath = L"..\\Bin\\ScriptGameMain_Debug.exe";
	configuration.releaseExePath = L"..\\Bin\\ScriptGameMain_Release.exe";

	GoEditor(TGE_PROJECT_SETTINGS_FILE, configuration);
	return 0;
}
