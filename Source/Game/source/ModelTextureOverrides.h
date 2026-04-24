#pragma once

#include <array>
#include <string>

#include <tge/EngineDefines.h>

struct MeshTextureOverrides
{
    static constexpr int kMaxMeshCount = MAX_MESHES_PER_MODEL;
    static constexpr int kTextureChannelCount = 4;

    std::array<std::array<std::string, kTextureChannelCount>, kMaxMeshCount> textures{};
};
