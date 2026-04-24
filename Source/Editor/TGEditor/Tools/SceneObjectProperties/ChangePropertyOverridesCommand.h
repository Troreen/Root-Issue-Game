#pragma once

#include <span>

#include <Commands/SceneCommandBase.h>
#include <tge/scene/Scene.h>

namespace Tga
{
	class ChangePropertyOverridesCommand : public SceneCommandBase
	{
	public:
		ChangePropertyOverridesCommand(uint32_t aObjectId, const SceneProperty& aNewPropertyDefintion, const SceneProperty& aOldPropertyDefintion);

		void Execute() override;
		void Undo() override;

		void GetModifiedObjects(std::vector<uint32_t>& outModifiedObjects, bool& outHasModifedSceneFile) const override;

	private:
		uint32_t mySceneObjectId;
		SceneProperty myOldValue;
		SceneProperty myNewValue;
	};
}
