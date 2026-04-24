
function TGETutorial(tutorial_name)
    project(tutorial_name)

	dependson { "External" }
	location "%{dirs.projectfiles}"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"

	debugdir "%{dirs.bin}"
	targetdir ("%{dirs.bin}")
	targetname("%{prj.name}_%{cfg.buildcfg}")
	objdir ("%{dirs.temp}/%{prj.name}/%{cfg.buildcfg}")

	verify_or_create_settings(tutorial_name)

	files {
		"**.h",
		"**.cpp",
	}

	includedirs { 
		dirs.external, 
		dirs.engine,
		"../TutorialCommon",
		"source/"
	}

	libdirs { 
		"%{dirs.dependencies}",
		"%{dirs.lib}"
	}

	links {
        "External",
		"Engine"
	}
	systemversion "latest"
	defines {"_CONSOLE"}
	
	filter "system:windows"
		--staticruntime "on"
		symbols "On"		
		systemversion "latest"
		warnings "Extra"
		flags { 
			"FatalCompileWarnings",
			--"MultiProcessorCompile"
		}
		defines {
			"WIN32",
			"_LIB", 
			"TGE_SYSTEM_WINDOWS"
		}

    filter {}

	filter "configurations:Debug"
		defines {"_DEBUG"}
		runtime "Debug"
		symbols "on"
		
	filter "configurations:Release"
		defines "_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Retail"
		defines "_Retail"
		runtime "Release"
		optimize "on"
end