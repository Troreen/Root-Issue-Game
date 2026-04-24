include "../../Premake/common.lua"

-------------------------------------------------------------
project "EngineTests"
	location (dirs.projectfiles)
	dependson { "External", "Engine" }
		
    kind ("SharedLib")

    language "C++"
	cppdialect "C++20"

	debugdir "%{dirs.bin}"
	targetdir ("%{dirs.bin}")
	targetname("%{prj.name}_%{cfg.buildcfg}")
	objdir ("%{dirs.temp}/%{prj.name}/%{cfg.buildcfg}")

	links {"External", "Engine"}

	includedirs { dirs.external, dirs.engine }
	libdirs { dirs.lib, dirs.dependencies }
    includedirs {"$(VCInstallDir)Auxiliary/VS/UnitTest/include"}
    libdirs { "$(VCInstallDir)Auxiliary/VS/UnitTest/lib"}
    
	files {
		"**.h",
		"**.cpp",
		"**.hlsl",
		"**.hlsli",
	}

	filter "system:windows"
--		kind "StaticLib"
		staticruntime "off"
		symbols "On"		
		systemversion "latest"
		warnings "Extra"
		--conformanceMode "On"
		--buildoptions { "/permissive" }
		flags { 
		--	"FatalWarnings", -- would be both compile and lib, the original didn't set lib
			"FatalCompileWarnings",
			"MultiProcessorCompile"
		}
		
		defines {
			"WIN32",
			"_LIB", 
			"TGE_SYSTEM_WINDOWS" 
		}