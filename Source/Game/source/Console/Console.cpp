#include "Console.h"
#include <imgui/imgui.h>
#include <sstream>

Console::Console()
{

}

void Console::InitCommands()
{
	CMD("System", "help", "help", "Displays this help message",
		[this](const std::vector<std::string>& /*args*/) {
			this->ListCommands();
		});

	CMD("System", "clear", "clear", "Clears the console",
		[this](const std::vector<std::string>& /*args*/) {
			myLog.clear();
			CONSOLE_LOG("Cleared!", LogLevel::Success);
		});

	CMD("Debug", "echo", "echo <text>", "Echos the input text.",
		[this](const std::vector<std::string>& args) {
			if (args.size() != 1)
				CONSOLE_LOG("Usage: echo <text>", LogLevel::Usage);
			else
				CONSOLE_LOG(args[0], LogLevel::Info);
		});
}

void Console::Draw()
{
	if (consoleActive == false)
		return;

	ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(20, 20, 20, 200)); // main window
	ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(20, 20, 20, 120));  // log area
	ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(40, 40, 40, 180));  // input
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));

	ImGui::PushStyleColor(ImGuiCol_ResizeGrip, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, IM_COL32(0, 0, 0, 0));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f); 
	ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 10.0f);  
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(600, 100));  

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	static float height = 100;  
	ImVec2 consoleSize = ImVec2(600, height);

	ImVec2 consolePos = ImVec2(
		viewport->WorkPos.x + 10,
		viewport->WorkPos.y + viewport->WorkSize.y - height - 10
	);

	ImGui::SetNextWindowPos(consolePos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(consoleSize, ImGuiCond_FirstUseEver);

	ImGui::SetNextWindowSizeConstraints(
		ImVec2(600, 100),   
		ImVec2(600, 600)   
	);


	ImGui::Begin("Console", nullptr,
		ImGuiWindowFlags_NoCollapse |
		//ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoTitleBar
	);

	height = ImGui::GetWindowHeight();

	ImGui::BeginChild("LogArea", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
	for (const auto& entry : myLog)
	{
		for (size_t i = 0; i < entry.parts.size(); ++i)
		{
			const auto& part = entry.parts[i];

			ImGui::TextColored(part.color, "%s", part.text.c_str());
			if (!part.tooltip.empty() && ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", part.tooltip.c_str());

			if (i < entry.parts.size() - 1)
				ImGui::SameLine(250);
		}
	}

	if (scrollToBottom)
	{
		ImGui::SetScrollHereY(1.0f);
		scrollToBottom = false;
	}
	ImGui::EndChild();

	if (focusInput)
	{
		ImGui::SetKeyboardFocusHere();
		focusInput = false;
	}

	if (ImGui::InputText("Input", myInputBuf, sizeof(myInputBuf),
		ImGuiInputTextFlags_EnterReturnsTrue))
	{
		ExecuteCommand(myInputBuf);
		strcpy_s(myInputBuf, sizeof(myInputBuf), "");
	}

	ImGui::End();
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(7);
}

void Console::AddLog(const std::string& aText, LogLevel aLevel)
{
	ImVec4 color;

	switch (aLevel)
	{
	case LogLevel::Info:    color = ImVec4(.6f, .6f, .6f, 1); break;
	case LogLevel::Warning: color = ImVec4(1, 1, 0, 1); break;
	case LogLevel::Error:   color = ImVec4(1, 0, 0, 1); break;
	case LogLevel::Command: color = ImVec4(1, 1, 1, 1); break;
	case LogLevel::Usage:   color = ImVec4(1, 1, .8f, 1); break;
	case LogLevel::Success: color = ImVec4(0, 1, 0, 1); break;
	}

	AddColoredLog(aText, color);
}

void Console::AddColoredLog(const std::string& aText, ImVec4 aColor)
{
	LogEntry entry;
	entry.parts.push_back({ aText, aColor });
	myLog.push_back(entry);
	scrollToBottom = true;
}

void Console::AddLogLine(const std::vector<LogPart>& parts)
{
	LogEntry entry;
	entry.parts = parts;
	myLog.push_back(entry);
	scrollToBottom = true;
}

void Console::ExecuteCommand(const std::string& line)
{
	AddLog("> " + line);

	auto tokens = Tokenize(line);
	if (tokens.empty()) return;

	auto it = myCommands.find(tokens[0]);
	if (it != myCommands.end())
	{
		tokens.erase(tokens.begin());
		it->second.fn(tokens);
	}
	else
	{
		AddLog("Unknown command: " + tokens[0]);
	}
}

void Console::RegisterCommand(const std::string& aCategory, 
	const std::string& aName,
	const std::string& aUsage,
	const std::string& aDescription,
	std::function<void(const std::vector<std::string>&)> aFunc)
{
	myCommands[aName] = Command{ aCategory, aName, aUsage, aDescription, aFunc };
}

void Console::ListCommands()
{
	std::unordered_map<std::string, std::vector<Command*>> categorized;
	for (auto& [name, cmd] : myCommands)
		categorized[cmd.category].push_back(&cmd);

	AddLog("Available commands:", LogLevel::Info);

	for (auto& [category, commands] : categorized)
	{
		AddLog(category, LogLevel::Command);

		for (auto* cmd : commands)
		{
			LogEntry entry;
			entry.parts.push_back({ "    " + cmd->name, ImVec4(1, 1, 0.9f, 1), cmd->usage});
			entry.parts.push_back({ cmd->description, ImVec4(0.6f, 0.6f, 0.6f, 1) });
			
			AddLogLine(entry.parts);
		}

		AddLog(""); 
	}
}

void Console::SetCommandUsage(const std::string& aName, const std::string& aUsage)
{
	auto it = myCommands.find(aName);
	if (it != myCommands.end())
		it->second.usage = aUsage;
}

std::vector<std::string> Console::Tokenize(const std::string& aStr)
{
	std::vector<std::string> tokens;
	std::istringstream iss(aStr);
	std::string token;
	while (iss >> token)
		tokens.push_back(token);
	return tokens;
}
