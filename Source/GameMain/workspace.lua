include "../../Premake/extensions.lua"

workspace "Game"
	location "../../"
	startproject "GameMain"
	architecture "x64"

	configurations {
		"Debug",
		"Release",
		"Retail"
	}

-- include for common stuff 
include "../../Premake/common.lua"

include (dirs.game)
include "."

group "Engine"
include (dirs.external)
include (dirs.engine)
include (dirs.engine_tests)