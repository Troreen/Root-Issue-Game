#include <GoEditor.h>

int main(const int /*argc*/, const char* /*argc*/[])
{
	EditorConfiguration configuration = {};
	configuration.debugExeName = "PlatformerGameMain_Debug.exe";
	configuration.releaseExeName = "PlatformerGameMain_Release.exe";
	configuration.debugExePath = L"..\\Bin\\PlatformerGameMain_Debug.exe";
	configuration.releaseExePath = L"..\\Bin\\PlatformerGameMain_Release.exe";

	GoEditor(TGE_PROJECT_SETTINGS_FILE, configuration);
	return 0;
}
