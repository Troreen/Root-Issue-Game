#include <GoEditor.h>

int main(const int /*argc*/, const char* /*argc*/[])
{
	EditorConfiguration configuration = {};
	configuration.enableVisualScripts = true;

	GoEditor(TGE_PROJECT_SETTINGS_FILE, configuration);
	return 0;
}
