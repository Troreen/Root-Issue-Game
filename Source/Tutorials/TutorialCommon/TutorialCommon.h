#pragma once
#include "stdafx.h"

namespace TutorialCommon
{
	void Init(std::wstring aAppName, bool aFullDebug = false, bool aVsyncEnable = true)
	{
		unsigned short windowWidth = 1920;
		unsigned short windowHeight = 1080;

		Tga::EngineConfiguration& cfg = Tga::Settings::GetEngineConfiguration();
		if (aFullDebug)
		{
			cfg.myActivateDebugSystems = Tga::DebugFeature::Fps | Tga::DebugFeature::Mem | Tga::DebugFeature::Filewatcher | Tga::DebugFeature::Cpu | Tga::DebugFeature::Drawcalls | Tga::DebugFeature::OptimizeWarnings;

		}
		else
		{
			cfg.myActivateDebugSystems = Tga::DebugFeature::Filewatcher;
		}

		cfg.myWindowSize = { windowWidth, windowHeight };
		cfg.myRenderSize = { windowWidth, windowHeight };
		cfg.myStartInFullScreen = false;
		cfg.myPreferedMultiSamplingQuality = Tga::MultiSamplingQuality::High;

		cfg.myApplicationName = aAppName;
		cfg.myEnableVSync = aVsyncEnable;

		if( !Tga::Engine::Start() )
		{
			ERROR_PRINT("Fatal error! Engine could not start!");
			system("pause");
	
		}
	}
}