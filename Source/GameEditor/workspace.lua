include "../../Premake/extensions.lua"

workspace "GameEditor"
	location "../../"
	startproject "GameEditor"
	architecture "x64"

	configurations {
		"Debug",
		"Release",
		"Retail"
	}

-- include for common stuff 
include "../../Premake/common.lua"


include (dirs.game)
include (dirs.gamemain)
include "."

group "Engine"
include (dirs.external)
include (dirs.engine)
include (dirs.engine_tests)

group "Editor"
include (dirs.editor)