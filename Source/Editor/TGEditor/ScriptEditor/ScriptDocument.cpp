#include "stdafx.h"

#include "ScriptDocument.h"

#include "Editor.h"
#include <ScriptEditor/ScriptEditor.h>
#include <tge/stringRegistry/StringRegistry.h>

#include <filesystem>

using namespace Tga;

void ScriptDocument::Init(std::string_view aPath)
{
	Document::Init(aPath);

	std::filesystem::path scriptPath = std::filesystem::path(aPath);
	scriptPath.replace_extension("");
	myScriptId = scriptPath.string();

	const std::string label = scriptPath.filename().string();
	myWindowName = label + "###Document:" + myPath;
	myImGuiName = StringRegistry::RegisterOrGetString(myWindowName);

	EditorScriptManager::GetInstance().OpenScript(myScriptId);
}

void ScriptDocument::Update(float aTimeDelta, InputManager& aInputManager)
{
	aTimeDelta;
	aInputManager;

	ImGui::SetNextWindowClass(Editor::GetEditor()->GetDocumentWindowClass());
	ImGui::SetNextWindowDockID(Editor::GetEditor()->GetDocumentDockSpaceId(), ImGuiCond_Once);

	bool open = true;
	if (ImGui::Begin(myWindowName.c_str(), &open))
	{
		if (myState == Document::State::Open && !open)
		{
			myState = Document::State::CloseRequested;
		}

		if (EditorScriptManager::GetInstance().OpenScript(myScriptId))
		{
			EditorScriptManager::GetInstance().DisplayEditor(myScriptId);
		}
		else
		{
			ImGui::Text("Could not open script: %s", myScriptId.c_str());
		}
	}
	ImGui::End();
}

void ScriptDocument::Save()
{
	EditorScriptManager::GetInstance().SaveAll();
}

void ScriptDocument::OnAction(CommandManager::Action aAction)
{
	aAction;
}
