#pragma once
#include <algorithm> 
#include <string>
#include <vector>
#include <functional>
#include <numeric>

#include "Console.h"

// Register a command with int argument and range checking
inline void RegisterIntCommand(const std::string& aCategory,
	const std::string& aName,
	const std::string& aUsage,
	const std::string& aDesc,
	std::function<void(int)> aFn,
	int aMin, int aMax)
{
	Console::Get().RegisterCommand(
		aCategory,
		aName,
		aUsage,
		aDesc,
		[aFn, aMin, aMax, aUsage](const std::vector<std::string>& args)
		{
			if (args.size() != 1)
			{
				CONSOLE_LOG("Usage: " + aUsage, LogLevel::Usage);
				return;
			}
			try
			{
				int value = std::stoi(args[0]);
				if (value < aMin || value > aMax)
				{
					CONSOLE_LOG("Value out of range (" + std::to_string(aMin) + "-" + std::to_string(aMax) + ")", LogLevel::Error);
					return;
				}
				aFn(value);
			}
			catch (...)
			{
				CONSOLE_LOG("Invalid number", LogLevel::Error);
			}
		});
}

// Register a command with float argument and range checking
inline void RegisterFloatCommand(const std::string& aCategory,
	const std::string& aName,
	const std::string& aUsage,
	const std::string& aDesc,
	std::function<void(float)> aFn,
	float aMin, float aMax)
{
	Console::Get().RegisterCommand(
		aCategory,
		aName,
		aUsage,
		aDesc,
		[aFn, aMin, aMax, aUsage](const std::vector<std::string>& args)
		{
			if (args.size() != 1)
			{
				CONSOLE_LOG("Usage: " + aUsage, LogLevel::Usage);
				return;
			}

			try
			{
				float value = std::stof(args[0]);
				if (value < aMin || value > aMax)
				{
					CONSOLE_LOG("Value out of range (" + std::to_string(aMin) + " - " + std::to_string(aMax) + ")", LogLevel::Error);
					return;
				}
				aFn(value);
			}
			catch (...)
			{
				CONSOLE_LOG("Invalid number", LogLevel::Error);
			}
		});
}

// Register a boolean command
inline void RegisterBoolCommand(const std::string& aCategory,
	const std::string& aName,
	const std::string& aUsage,
	const std::string& aDesc,
	std::function<void(bool)> aFn)
{
	Console::Get().RegisterCommand(
		aCategory,
		aName,
		aUsage,
		aDesc,
		[aFn, aUsage](const std::vector<std::string>& args)
		{
			if (args.size() != 1)
			{
				CONSOLE_LOG("Usage: " + aUsage, LogLevel::Usage);
				return;
			}
			std::string val = args[0];
			std::transform(val.begin(), val.end(), val.begin(), ::tolower);
			if (val == "true") aFn(true);
			else if (val == "false") aFn(false);
			else CONSOLE_LOG("Invalid boolean value (true/false)", LogLevel::Error);
		});
}

// Register an enum command
inline void RegisterEnumCommand(const std::string& aCategory,
	const std::string& aName,
	const std::string& aUsage,
	const std::string& aDesc,
	std::function<void(const std::string&)> aFn,
	const std::vector<std::string>& allowedValues)
{
	Console::Get().RegisterCommand(
		aCategory,
		aName,
		aUsage,
		aDesc,
		[aFn, allowedValues, aUsage](const std::vector<std::string>& args)
		{
			if (args.size() != 1)
			{
				CONSOLE_LOG("Usage: " + aUsage, LogLevel::Usage);
				return;
			}

			std::string val = args[0];
			auto it = std::find(allowedValues.begin(), allowedValues.end(), val);
			if (it == allowedValues.end())
			{
				CONSOLE_LOG("Invalid option. Allowed: " + std::accumulate(
					std::next(allowedValues.begin()), allowedValues.end(), allowedValues[0],
					[](std::string a, std::string b) { return a + ", " + b; }),
					LogLevel::Error);
				return;
			}

			aFn(val);
		});
}

// Register a multi-argument command
inline void RegisterMultiArgCommand(
	const std::string& aCategory,
	const std::string& aName,
	const std::string& aUsage,
	const std::string& aDesc,
	std::function<void(const std::vector<std::string>&)> fn)
{
	Console::Get().RegisterCommand(aCategory, aName, aUsage, aDesc, fn);
}

// Register a raw/no-arg command
inline void RegisterCommand(const std::string& aCategory,
	const std::string& aName,
	const std::string& aUsage,
	const std::string& aDesc,
	std::function<void(const std::vector<std::string>&)> aFn)
{
	Console::Get().RegisterCommand(aCategory, aName, aUsage, aDesc, aFn);
}