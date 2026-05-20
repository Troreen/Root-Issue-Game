#pragma once

#include <Document/Document.h>

namespace Tga
{
	class ScriptDocument final : public Document
	{
	public:
		void Init(std::string_view aPath) override;
		void Update(float aTimeDelta, InputManager& aInputManager) override;
		void Save() override;
		void OnAction(CommandManager::Action aAction) override;

	private:
		std::string myScriptId;
		std::string myWindowName;
	};
}
