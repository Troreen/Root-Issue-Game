#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <sstream>
#include <iostream>
#include <imgui/imgui.h>

enum class LogLevel { Info, Warning, Error, Command, Usage, Success };

struct LogPart {
	std::string text;
	ImVec4 color;
	std::string tooltip;
};

struct LogEntry {
	std::vector<LogPart> parts;
};

class Console
{
public:
	static Console& Get()
	{
		static Console instance;
		return instance;
	}

	Console();
	Console(const Console&) = delete;
	void operator=(const Console&) = delete;

	void InitCommands();
	void Draw();

	void Toggle() { consoleActive = !consoleActive; }
	void ToggleFocus() { focusInput = !focusInput; }
	void SetFocus(bool focus) { focusInput = focus; }
	bool GetFocus() const { return focusInput; }
	bool IsActive() const { return consoleActive; }

	void AddLog(const std::string& aText, LogLevel aLevel = LogLevel::Info);
	void AddColoredLog(const std::string& aText, ImVec4 aColor);
	void AddLogLine(const std::vector<LogPart>& parts);

	void ExecuteCommand(const std::string& line);
	void RegisterCommand(const std::string& aCategory,
		const std::string& aName,
		const std::string& aUsage,
		const std::string& aDescription,
		std::function<void(const std::vector<std::string>&)> aFunc);
	void ListCommands();
	void SetCommandUsage(const std::string& aName, const std::string& aUsage);

private:
	char myInputBuf[256] = {};
	std::vector<LogEntry> myLog;

	struct Command
	{
		std::string category;
		std::string name;
		std::string usage;
		std::string description;
		std::function<void(const std::vector<std::string>&)> fn;
	};

	std::unordered_map<std::string, Command> myCommands;
	std::vector<std::string> Tokenize(const std::string& aStr);

	bool scrollToBottom = false;
	bool consoleActive = false;
	bool focusInput = false;
};

// Simple macro for registering commands
#define CMD(category, name, usage, description, lambda) \
    Console::Get().RegisterCommand(category, name, usage, description, lambda)

#define CONSOLE_LOG(text, level) Console::Get().AddLog(text, level)