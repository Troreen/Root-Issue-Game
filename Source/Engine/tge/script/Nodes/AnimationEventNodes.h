#pragma once

#include <span>

namespace Tga
{
	void RegisterAnimationEventNodes();
	std::span<const char* const> GetAnimationEventSfxIds();
	std::span<const char* const> GetAnimationEventCombatTeamIds();
	std::span<const char* const> GetAnimationEventAttackTypeIds();
	std::span<const char* const> GetAnimationEventCollisionShapeIds();
	std::span<const char* const> GetAnimationEventTargetLayerIds();
}
