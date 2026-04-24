#pragma once
#include <array>
#include <unordered_map>
#include <tge/Graphics/DepthBuffer.h>
#include <tge/Graphics/RenderTarget.h>
#include <tge/Graphics/GraphicsStateStack.h>
#include <tge/Math/Color.h>
#include <tge/Math/Vector.h>
#include <thread>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11Debug;
struct ID3D11PixelShader;
struct ID3D11VertexShader;
struct ID3D11GeometryShader;

namespace Tga
{
class WindowsWindow;


struct PixelShader
{
	ComPtr<ID3D11PixelShader> shader;
};
struct VertexShader
{
	ComPtr<ID3D11VertexShader> shader;
	std::string data;
};
struct ComputeShader
{
	ComPtr<ID3D11ComputeShader> shader;
};

// DirectX 11 Framework. Shorthand to make it easier to deal with.
class DX11
{
	static WindowsWindow* myWindowHandler;
	
public:
	enum class ShaderType
	{
		Pixel,
		Vertex,
		Compute
	};

	DX11();
	~DX11();

	bool Init(WindowsWindow* aWindowHandler);
	bool ResizeToWindowSize();
	void BeginFrame(Color aClearColor);
	void EndFrame(bool aEnableVsync = true);

	static bool IsOnSameThreadAsEngine();

	static Vector2ui GetResolution();

	static ID3D11Device* Device;
	static ID3D11DeviceContext* Context;
	static IDXGISwapChain* SwapChain;
	static RenderTarget* BackBuffer;
	static RenderTarget* BackBufferNoSrgbConversion; // ImGui works in sRGB, so requires sRGB access to the backbuffer. Do not use for anything else
	static DepthBuffer* DepthBuffer;

	static const PixelShader* LoadPixelShader(const char* aShaderPath);
	static const VertexShader* LoadVertexShader(const char* aShaderPath);
	static const ComputeShader* LoadComputeShader(const char* aShaderPath);

	static void ResetDrawCallCounter() { PreviousDrawCallCount = DrawCallCount; DrawCallCount = 0; };
	static void LogDrawCall() { DrawCallCount++; }
	static int GetPreviousDrawCallCount() { return PreviousDrawCallCount; }

	static void OnShaderFileChanged(const std::string& aShaderPath, ShaderType aShaderType);

private:
	static const PixelShader* ForceLoadPixelShader(const char* aShaderPath, bool addToFileWatcher = true);
	static const VertexShader* ForceLoadVertexShader(const char* aShaderPath, bool addToFileWatcher = true);
	static const ComputeShader* ForceLoadComputeShader(const char* aShaderPath, bool addToFileWatcher = true);

	static int DrawCallCount;
	static int PreviousDrawCallCount;
	static bool IsCreated;
	static std::thread::id RenderThreadId;

	ComPtr<ID3D11Device> myDevice;
	ComPtr<ID3D11DeviceContext> myContext;
	ComPtr<IDXGISwapChain> mySwapChain;
	RenderTarget myBackBuffer;
	RenderTarget myBackBufferNoSrgbConversion;
	Tga::DepthBuffer myDepthBuffer;

	ComPtr<ID3D11Debug> myD3dDebug;

	static std::unordered_map<std::string, PixelShader> myLoadedPixelShaders;
	static std::unordered_map<std::string, VertexShader> myLoadedVertexShaders;
	static std::unordered_map<std::string, ComputeShader> myLoadedComputeShaders;
};

} // namespace Tga