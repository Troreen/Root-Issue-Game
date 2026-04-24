/*
Engine
This class is the interface between game logic and rendering. 
It takes care of creating everything necessary for window handling and rendering
*/

#pragma once

#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX 
#include <windows.h>

#include <functional>
#include <tge/math/color.h>
#include <tge/math/vector2.h>
#include <tge/render/RenderCommon.h>
#include <chrono>
#include "StepTimer.h"
#include <tge/EngineDefines.h>

namespace Tga
{
    class FileWatcher;
    class DX11;

#ifndef _RETAIL
    class ImGuiInterface;
#endif // !_RETAIL
}

#define TGA_DEFAULT_CRYSTAL_BLUE { 3.0f / 255.0f, 153.0f / 255.0f, 176.0f / 255.0f, 1.0f }

namespace Tga
{
	using callback_function_wndProc = std::function<LRESULT(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)>;
    using callback_function_log    = std::function<void( std::string )>;
    using callback_function_error  = std::function<void( std::string )>;

    enum class DebugFeature
    {
        None = 0,
        Fps = 1 << 0,
        Mem = 1 << 1,
        Drawcalls = 1 << 2,
        Cpu = 1 << 3,
        FpsGraph = 1 << 4,
        Filewatcher = 1 << 5,
        OptimizeWarnings = 1 << 6,
        MemoryTrackingStackTraces = 1 << 7,
        MemoryTrackingAllAllocations =  1 << 8,
        All = (1 << 9) - 1,
    };
    inline DebugFeature operator|(DebugFeature lhs, DebugFeature rhs)
    {
        return static_cast<DebugFeature>(static_cast<std::underlying_type<DebugFeature>::type>(lhs) | static_cast<std::underlying_type<DebugFeature>::type>(rhs));
    }
    inline DebugFeature operator&(DebugFeature lhs, DebugFeature rhs)
    {
        return static_cast<DebugFeature>(static_cast<std::underlying_type<DebugFeature>::type>(lhs) & static_cast<std::underlying_type<DebugFeature>::type>(rhs));
    }
	enum class MultiSamplingQuality
	{
		Off,	
		Low = 1,
		Medium = 2,
		High = 3,
	};

    struct EngineConfiguration
    {
        EngineConfiguration()
        { 
            myHwnd							= nullptr; 
            myHInstance						= nullptr;  
            myErrorFunction					= nullptr; 
			myLogFunction					= nullptr;
			myWinProcCallback				= nullptr;
            myWindowSize                    = { 1280, 720 };
            myEnableVSync					= false; 
            myRenderSize                    = myWindowSize;
            myStartInFullScreen				= false;
            myStartMaximized                = false;
			myClearColor					= TGA_DEFAULT_CRYSTAL_BLUE;
            myApplicationName               = L"TGA - Engagemang, Respect och Nyfikenhet!";
            myBorderless                    = false;
            myActivateDebugSystems			= DebugFeature::Fps | DebugFeature::Mem;
			myPreferedMultiSamplingQuality	= MultiSamplingQuality::Off;
        }

		callback_function_wndProc myWinProcCallback;
        callback_function_log myLogFunction;
        callback_function_error myErrorFunction;

		/* How big should the window be? */
        Vector2ui myWindowSize;

		/* What resolution should we render everything in?*/
        Vector2ui myRenderSize;

        /* Will show the FPS and memory text*/
        DebugFeature myActivateDebugSystems;
        Color myClearColor;
        HWND *myHwnd;
        HINSTANCE myHInstance;
        std::wstring myApplicationName;
        bool myEnableVSync;
        bool myStartInFullScreen;
        bool myStartMaximized;
        bool myBorderless;
        
		MultiSamplingQuality myPreferedMultiSamplingQuality;
    };


    class Engine
    {
        friend class TextureManager;
        friend class Renderer;
        friend class Shader;
        friend class ErrorManager;
        friend class GraphicsEngine;
		friend class WindowsWindow;
		friend class ErrorManager;
		friend class CustomShapeDrawer;
		friend class LineDrawer;
		friend class DebugDrawer;
		friend class CustomShape;
		friend class Texture;
		friend class Text;
		friend class TextService;
		friend class Video;
		friend class Light;
		friend class ModelShape;

    public:
        Engine &operator =( const Engine &anOther ) = delete;
		static bool Start();
		static void Shutdown();

        static Engine* GetInstance() {return myInstance;}

        TextureManager& GetTextureManager() const {return *myTextureManager;}
        DebugDrawer& GetDebugDrawer() const {return *myDebugDrawer;}
		ErrorManager& GetErrorManager() const { return *myErrorManager; }
        TextService& GetTextService() const { return *myTextService; }

		Vector2ui GetWindowSize() const { return myWindowConfiguration.myWindowSize; }
		Vector2ui GetRenderSize() const { return myWindowConfiguration.myRenderSize; }

        float GetRenderSizeRatio() const;
        float GetRenderSizeRatioInversed() const;

		float GetDeltaTime() const { return myDeltaTime; }
        float GetTotalTime() const { return myTotalTime; }
        float GetUnscaledDeltaTime() const { return myUnscaledDeltaTime; }
        float GetUnscaledTotalTime() const { return myUnscaledTotalTime; }
        HWND* GetHWND() const;
        HINSTANCE GetHInstance() const;
      
		bool IsDebugFeatureOn(DebugFeature aFeature) const;

		void SetResolution(const Vector2ui &aResolution);

        void SetFullScreen(bool aFullScreen);
		
		void SetClearColor(const Color& aClearColor);
        const Color& GetClearColor() const { return myWindowConfiguration.myClearColor; }
		// If you want to manually tell the engine to render instead of the callback function with the (myUpdateFunctionToCall)
		bool BeginFrame();
		void EndFrame(void);

		GraphicsEngine& GetGraphicsEngine() const { return *myGraphicsEngine; }

        FileWatcher* GetFileWatcher() { return myFileWatcher.get(); }

    private:    // Private interface
        Engine();
        ~Engine();

        static void DestroyInstance();

		WindowsWindow& GetWindow() const { return *myWindow; }
		
		bool InternalStart();

        void CalculateRatios();
		void SetWantToUpdateSize() { myWantToUpdateSize = true; }

		void UpdateWindowSizeChanges();

		Vector2f GetRenderSizeRatioVec() const;
		Vector2f GetRenderSizeRatioInversedVec() const;

    private:    // Private data
        static Engine* myInstance;

        std::unique_ptr<WindowsWindow> myWindow;
        std::unique_ptr<DX11> myDx11;
        std::unique_ptr<GraphicsEngine> myGraphicsEngine;
        std::unique_ptr<TextureManager> myTextureManager;
        std::unique_ptr<DebugDrawer> myDebugDrawer;
        std::unique_ptr<TextService> myTextService;

        EngineConfiguration myWindowConfiguration;

        std::unique_ptr<ErrorManager> myErrorManager;
        std::unique_ptr<FileWatcher> myFileWatcher;
		bool myWantToUpdateSize;

        bool myRunEngine;
        float myRenderSizeRatio;
        float myRenderSizeRatioInversed;
		Vector2f myRenderSizeRatioVec;
		Vector2f myRenderSizeRatioInversedVec;

		std::chrono::steady_clock::time_point myStartOfTime;

        float myTotalTime;
        float myDeltaTime;

        float myUnscaledTotalTime;
        float myUnscaledDeltaTime;

		DX::StepTimer myTimer;

		bool myShouldExit; // Only used when using beginframe and endframe
    };
}
