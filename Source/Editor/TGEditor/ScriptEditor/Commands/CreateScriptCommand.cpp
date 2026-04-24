#include "CreateScriptCommand.h"

#include <ScriptEditor/ScriptEditor.h>
#include <tge/script/Script.h>
#include <tge/script/ScriptNodeBase.h>

using namespace Tga;

void Tga::CreateScriptCommand::ExecuteImpl()
{
	EditorScriptManager::GetInstance().MarkScriptAsAdded(myName);
}

void Tga::CreateScriptCommand::UndoImpl()
{
	EditorScriptManager::GetInstance().MarkScriptAsRemoved(myName);
}