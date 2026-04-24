#include "stdafx.h"
#include <tge/graphics/DX11.h>

#include <fstream>
#include <tge/windows/WindowsWindow.h>
#include <nlohmann/json.hpp>
#include <tge/util/StringCast.h>

#ifndef _RETAIL

#include <tge/filewatcher/FileWatcher.h>

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

#endif

using namespace Tga;

#define REPORT_DX_WARNINGS

ID3D11Device* DX11::Device;
ID3D11DeviceContext* DX11::Context;
IDXGISwapChain* DX11::SwapChain;
RenderTarget* DX11::BackBuffer;
RenderTarget* DX11::BackBufferNoSrgbConversion;
DepthBuffer* DX11::DepthBuffer;

WindowsWindow* DX11::myWindowHandler;

bool DX11::IsCreated;
std::thread::id DX11::RenderThreadId;
int DX11::DrawCallCount = 0;
int DX11::PreviousDrawCallCount = 0;

std::unordered_map<std::string, PixelShader> DX11::myLoadedPixelShaders;
std::unordered_map<std::string, VertexShader> DX11::myLoadedVertexShaders;
std::unordered_map<std::string, ComputeShader> DX11::myLoadedComputeShaders;

DX11::DX11()
{
	assert(IsCreated == false);
	IsCreated = true;

	Device = nullptr;
	Context = nullptr;
	SwapChain = nullptr;
	BackBuffer = nullptr;
	BackBufferNoSrgbConversion = nullptr;
	DepthBuffer = nullptr;
}

DX11::~DX11()
{
	assert(IsCreated == true);
	IsCreated = false;

#ifdef REPORT_DX_WARNINGS
	if (myD3dDebug)
	{
		myD3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL);
	}
#endif

	Device = nullptr;
	Context = nullptr;
	SwapChain = nullptr;
	BackBuffer = nullptr;
	BackBufferNoSrgbConversion = nullptr;
	DepthBuffer = nullptr;
}

IDXGIAdapter* FindBestAdapter()
{
	HRESULT result = S_OK;
	IDXGIFactory* factory;

	DXGI_MODE_DESC* displayModeList = nullptr;

	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	if (FAILED(result))
	{
		return nullptr;
	}
	// Use the factory to create an adapter for the primary graphics interface (video card).
	IDXGIAdapter* usingAdapter = nullptr;
	int adapterIndex = 0;
	std::vector<DXGI_ADAPTER_DESC> myAdapterDescs;
	std::vector<IDXGIAdapter*> myAdapters;
	while (factory->EnumAdapters(adapterIndex, &usingAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC adapterDesc;
		usingAdapter->GetDesc(&adapterDesc);
		myAdapterDescs.push_back(adapterDesc);
		myAdapters.push_back(usingAdapter);
		++adapterIndex;
	}

	if (adapterIndex == 0)
	{
		return nullptr;
	}

	INFO_PRINT("%s", "Video card(s) detected: ");
	for (DXGI_ADAPTER_DESC desc : myAdapterDescs)
	{
		int memory = (int)(desc.DedicatedVideoMemory / 1024 / 1024);
		INFO_PRINT("	%ls%s%i%s", desc.Description, " Mem: ", memory, "Mb");
		memory;
	}

	DXGI_ADAPTER_DESC usingAdapterDesc = myAdapterDescs[0];
	usingAdapter = myAdapters[0];

	INFO_PRINT("%s", "Detecting best card...");


	const std::wstring nvidia = L"NVIDIA";
	const std::wstring ati = L"ATI";

	int memory = (int)(usingAdapterDesc.DedicatedVideoMemory / 1024 / 1024);
	int mostMem = 0;

	for (unsigned int i = 0; i < myAdapterDescs.size(); i++)
	{
		DXGI_ADAPTER_DESC desc = myAdapterDescs[i];
		memory = (int)(desc.DedicatedVideoMemory / 1024 / 1024);
		std::wstring name = desc.Description;
		if (name.find(nvidia) != std::wstring::npos || name.find(ati) != std::wstring::npos)
		{
			if (memory > mostMem)
			{
				mostMem = memory;
				usingAdapterDesc = desc;
				usingAdapter = myAdapters[i];
			}
		}
	}

	INFO_PRINT("%s%ls%s%i", "Using graphic card: ", usingAdapterDesc.Description, " Dedicated Mem: ", mostMem);

	// Get the adapter (video card) description.
	result = usingAdapter->GetDesc(&usingAdapterDesc);
	if (FAILED(result))
	{
		return nullptr;
	}

	// Release the display mode list.
	delete[] displayModeList;
	displayModeList = 0;

	// Release the factory.
	factory->Release();
	factory = 0;

	return usingAdapter;
}


bool DX11::Init(WindowsWindow* aWindowHandler)
{
	myWindowHandler = aWindowHandler;
	
	HRESULT result;

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = myWindowHandler->GetWindowHandle();
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = true;

	UINT creationFlags = 0;
#if defined(_DEBUG)
#if defined(REPORT_DX_WARNINGS)
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
#endif

	IDXGIAdapter* adapter = FindBestAdapter();
	if (!adapter)
	{
		return false;
	}

	result = D3D11CreateDeviceAndSwapChain(
		adapter,
		D3D_DRIVER_TYPE_UNKNOWN,
		nullptr,
		creationFlags,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&swapChainDesc,
		&mySwapChain,
		&myDevice,
		nullptr,
		&myContext
	);

#if defined(_DEBUG)
#if defined(REPORT_DX_WARNINGS)
	if (FAILED(result))
	{
		ERROR_PRINT("%s", "Device could not create itself in debug mode, trying without debug layer... If you have Win10, try this:  Settings panel -> System -> Apps & features -> Manage optional Features -> Add a feature -> Select ""Graphics Tools""");
		creationFlags = 0;

		INFO_PRINT("%s", "Creating device without debug layer");
		result = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			creationFlags,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&swapChainDesc,
			&mySwapChain,
			&myDevice,
			nullptr,
			&myContext
		);
	}
#endif
#endif

	if (FAILED(result))
	{
		return false;
	}

	myD3dDebug = nullptr;
#ifdef _DEBUG
	if (SUCCEEDED(myDevice.As(&myD3dDebug)))
	{
		ComPtr<ID3D11InfoQueue> d3dInfoQueue = nullptr;
		if (SUCCEEDED(myD3dDebug.As(&d3dInfoQueue)))
		{
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, false);

			D3D11_MESSAGE_ID hide[] =
			{
				D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
				// Add more message IDs here as needed
			};

			D3D11_INFO_QUEUE_FILTER filter;
			memset(&filter, 0, sizeof(filter));
			filter.DenyList.NumIDs = _countof(hide);
			filter.DenyList.pIDList = hide;
			d3dInfoQueue->AddStorageFilterEntries(&filter);
		}
	}
#endif

	Device = myDevice.Get();
	Context = myContext.Get();
	SwapChain = mySwapChain.Get();

	ID3D11Texture2D* backBufferTexture;
	result = mySwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferTexture);
	myBackBuffer = RenderTarget::Create(backBufferTexture, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
	myBackBufferNoSrgbConversion = RenderTarget::Create(backBufferTexture);
	D3D11_TEXTURE2D_DESC textureDesc;
	backBufferTexture->GetDesc(&textureDesc);
	backBufferTexture->Release();
	myDepthBuffer = DepthBuffer::Create({ static_cast<unsigned int>(textureDesc.Width), static_cast<unsigned int>(textureDesc.Height) });

	myBackBuffer.SetAsActiveTarget(&myDepthBuffer);

	BackBuffer = &myBackBuffer;
	BackBufferNoSrgbConversion = &myBackBufferNoSrgbConversion;
	DepthBuffer = &myDepthBuffer;

	RenderThreadId = std::this_thread::get_id();

	return true;
}

bool DX11::ResizeToWindowSize()
{
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	myContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
	myContext->OMSetDepthStencilState(0, 0);
	myContext->ClearState();
	
	if (!mySwapChain)
	{
		return false;
	}

	myBackBuffer = RenderTarget();
	myBackBufferNoSrgbConversion = RenderTarget();
	myDepthBuffer = Tga::DepthBuffer();

	myContext->Flush();
	if (mySwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0) != S_OK)
	{
		ERROR_PRINT("%s", "Could not resize buffers!");
		return false;
	}

	BackBuffer = nullptr;
	DepthBuffer = nullptr;

	ID3D11Texture2D* backBufferTexture = nullptr;
	if (mySwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferTexture) != S_OK)
	{
		ERROR_PRINT("%s", "Could not resize buffers!");
		return false;
	}

	if (!backBufferTexture)
	{
		return false;
	}

	myBackBuffer = RenderTarget::Create(backBufferTexture, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
	myBackBufferNoSrgbConversion = RenderTarget::Create(backBufferTexture);

	D3D11_TEXTURE2D_DESC textureDesc;
	backBufferTexture->GetDesc(&textureDesc);
	backBufferTexture->Release();
	myDepthBuffer = DepthBuffer::Create({ static_cast<unsigned int>(textureDesc.Width), static_cast<unsigned int>(textureDesc.Height) });
	myBackBuffer.SetAsActiveTarget(&myDepthBuffer);

	BackBuffer = &myBackBuffer;
	BackBufferNoSrgbConversion = &myBackBufferNoSrgbConversion;
	DepthBuffer = &myDepthBuffer;

	return true;
}

void DX11::BeginFrame(Color aClearColor)
{
	myBackBuffer.Clear(aClearColor.AsLinearVec4());
	myDepthBuffer.Clear(1.0f, 0);

	DX11::BackBuffer->SetAsActiveTarget();
}

void DX11::EndFrame(bool aEnableVSync)
{
	if (aEnableVSync)
	{
		DX11::SwapChain->Present(1, 0);
	}
	else
	{
		DX11::SwapChain->Present(0, 0);
	}
}

bool DX11::IsOnSameThreadAsEngine()
{
	return RenderThreadId == std::this_thread::get_id();
}

Vector2ui DX11::GetResolution()
{
	return BackBuffer->GetResolution();
}

#ifndef _RETAIL

std::string GetFileTimestamp(const std::string& filePath)
{
	std::filesystem::file_time_type fileTime = std::filesystem::last_write_time(filePath);
	auto systemTimePoint = std::chrono::clock_cast<std::chrono::system_clock>(fileTime);
	auto fileSystemTime = std::chrono::system_clock::to_time_t(systemTimePoint);

	std::ostringstream oss;
	oss << std::put_time(std::gmtime(&fileSystemTime), "%Y-%m-%dT%H:%M:%SZ");
	return oss.str();
};

// Function to check if a file has changed based on its recorded timestamp
bool HasFileChanged(const std::string& filePath, const std::string& recordedTimestamp)
{
	if (!std::filesystem::exists(filePath))
	{
		return true; // Assume changed if the file does not exist
	}

	auto fileTime = std::filesystem::last_write_time(filePath);
	auto systemTimePoint = std::chrono::clock_cast<std::chrono::system_clock>(fileTime);
	auto fileSystemTime = std::chrono::system_clock::to_time_t(systemTimePoint);

	if (recordedTimestamp.empty())
	{
		return true; // Assume changed if no recorded timestamp
	}

	// Parse the recorded timestamp (ISO8601 format)
	std::tm recordedTime = {};
	std::istringstream ss(recordedTimestamp);
	ss >> std::get_time(&recordedTime, "%Y-%m-%dT%H:%M:%SZ");

	return std::difftime(fileSystemTime, _mkgmtime(&recordedTime)) != 0;
}


struct IncludeHandler : public ID3DInclude
{
	std::string sourcePath;
	std::set<std::string> includedFiles;

	HRESULT Open(
		D3D_INCLUDE_TYPE,
		LPCSTR pFileName,
		LPCVOID,
		LPCVOID* ppData,
		UINT* pBytes) override
	{
		std::string resolvedPath = Settings::ResolveAssetPath(sourcePath + "/" + pFileName);
		if (resolvedPath.empty())
		{
			resolvedPath = Settings::ResolveAssetPath(pFileName);
			if (resolvedPath.empty())
			{
				std::cerr << "Failed to open shader include file: " << pFileName << std::endl;
				return E_FAIL;
			}
		}

		std::ifstream file(resolvedPath, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			std::cerr << "Failed to open shader include file: " << pFileName << std::endl;
			return E_FAIL;
		}

		includedFiles.insert(pFileName);

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);
		char* buffer = new char[size];
		file.read(buffer, size);
		file.close();

		*ppData = buffer;
		*pBytes = static_cast<UINT>(size);
		return S_OK;
	}

	HRESULT Close(LPCVOID pData) override
	{
		delete[] reinterpret_cast<const char*>(pData);
		return S_OK;
	}
};

const char* GetShaderModel(DX11::ShaderType aShaderType)
{
	switch (aShaderType)
	{
	case DX11::ShaderType::Pixel:
		return "ps_5_0";
	case DX11::ShaderType::Vertex:
		return "vs_5_0";
	case DX11::ShaderType::Compute:
		return "cs_5_0";
	}
	return nullptr;
}


bool CompileShaderIfChanged(const std::string& shaderWithoutExtension, const std::string& aRelShaderFile, const std::string& aAbsShaderFile, const std::string& aBuildDependencyFile, const std::string& aBinaryOutputFile, DX11::ShaderType aShaderType, bool addFileWatcher = true)
{
	std::string shaderRelBasePath = std::filesystem::path(aRelShaderFile).parent_path().string();

	bool needsRecompile = true;
	if (std::filesystem::exists(aBuildDependencyFile))
	{
		std::ifstream inputFile(aBuildDependencyFile);
		if (!inputFile.is_open())
		{
			std::cerr << "Failed to open shader build dependency file." << std::endl;
			return false;
		}

		nlohmann::json buildDependencyData;
		inputFile >> buildDependencyData;

		// Check if the shader or any dependencies have changed
		needsRecompile = HasFileChanged(aAbsShaderFile, buildDependencyData["timestamp"]);

		for (const auto& dependency : buildDependencyData["dependencies"].items())
		{
			const std::string& depFile = dependency.key();
			const std::string& depTimestamp = dependency.value();

			std::string resolvedPath = Settings::ResolveAssetPath(shaderRelBasePath + "/" + depFile);
			if (resolvedPath.empty())
			{
				resolvedPath = Settings::ResolveAssetPath(depFile);
			}
			if (HasFileChanged(resolvedPath, depTimestamp))
			{
				needsRecompile = true;
				break;
			}
		}
	}

	bool compilationSucceded = true;
	if (needsRecompile)
	{
		// Recompile the shader
		ID3DBlob* shaderBlob = nullptr;
		ID3DBlob* errorBlob = nullptr;
		IncludeHandler includeHandler;

		includeHandler.sourcePath = shaderRelBasePath;
		
		unsigned int flags = D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS;

		std::wstring wFilename = string_cast<std::wstring>(aAbsShaderFile);
		HRESULT hr = D3DCompileFromFile(
			wFilename.c_str(),
			nullptr, &includeHandler,
			"main",
			GetShaderModel(aShaderType),
			flags, 0,
			&shaderBlob, &errorBlob);

		if (FAILED(hr))
		{
			if (errorBlob)
			{
				std::cerr << "Shader compilation error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
				errorBlob->Release();
			}
			if (shaderBlob)
			{
				shaderBlob->Release();
			}
			compilationSucceded = false;
		}
		else
		{
			// Save the compiled shader to a binary file (CSO)
			std::filesystem::path parentPath = std::filesystem::path(aBinaryOutputFile).parent_path();
			std::filesystem::create_directories(parentPath);

			std::ofstream binaryFile(aBinaryOutputFile, std::ios::binary);
			if (!binaryFile.is_open())
			{
				std::cerr << "Failed to save compiled shader to binary file." << std::endl;
				shaderBlob->Release();
				return false;
			}
			binaryFile.write(reinterpret_cast<const char*>(shaderBlob->GetBufferPointer()), shaderBlob->GetBufferSize());
			binaryFile.close();

			// Release the shader blob
			shaderBlob->Release();

			{
				nlohmann::json buildDependencyData;
				buildDependencyData["timestamp"] = GetFileTimestamp(aAbsShaderFile);

				for (auto& dependency : includeHandler.includedFiles)
				{
					std::string resolvedPath = Settings::ResolveAssetPath(shaderRelBasePath + "/" + dependency);
					if (resolvedPath.empty())
					{
						resolvedPath = Settings::ResolveAssetPath(dependency);
					}
					buildDependencyData["dependencies"][dependency] = GetFileTimestamp(resolvedPath);
				}

				// Save the updated JSON file
				std::ofstream outputFile(aBuildDependencyFile);
				if (!outputFile.is_open())
				{
					std::cerr << "Failed to save build dependency file." << std::endl;
					return false;
				}
				outputFile << buildDependencyData.dump(4); // Pretty print with 4 spaces
			}
		}
	}

	if (Engine::GetInstance()->IsDebugFeatureOn(DebugFeature::Filewatcher) && addFileWatcher)
	{
		Engine::GetInstance()->GetFileWatcher()->WatchFileChange(aAbsShaderFile, std::bind(&DX11::OnShaderFileChanged, shaderWithoutExtension, aShaderType));

		nlohmann::json buildDependencyData;
		std::ifstream inputFile(aBuildDependencyFile);
		if (inputFile.is_open())
		{
			inputFile >> buildDependencyData;

			for (const auto& dependency : buildDependencyData["dependencies"].items())
			{
				const std::string& depFile = dependency.key();

				std::string resolvedPath = Settings::ResolveAssetPath(shaderRelBasePath + "/" + depFile);
				if (resolvedPath.empty())
				{
					resolvedPath = Settings::ResolveAssetPath(depFile);
				}

				if (!resolvedPath.empty())
				{
					Engine::GetInstance()->GetFileWatcher()->WatchFileChange(resolvedPath, std::bind(&DX11::OnShaderFileChanged, shaderWithoutExtension, aShaderType));

				}
			}
		}
	}

	return true;
}

void DX11::OnShaderFileChanged(const std::string& aShaderPath, ShaderType aShaderType)
{
	switch (aShaderType)
	{
	case ShaderType::Pixel:
		ForceLoadPixelShader(aShaderPath.c_str(), false);
		break;
	case ShaderType::Vertex:
		ForceLoadVertexShader(aShaderPath.c_str(), false);
		break;
	case ShaderType::Compute:
		ForceLoadComputeShader(aShaderPath.c_str(), false);
		break;
	default:
		break;
	}
}

#endif // #ifndef _RETAIL

const PixelShader* DX11::LoadPixelShader(const char* aShaderPath)
{
	auto it = myLoadedPixelShaders.find(aShaderPath);
	if (it != myLoadedPixelShaders.end())
		return &(it->second);

	return ForceLoadPixelShader(aShaderPath);
}

const VertexShader* DX11::LoadVertexShader(const char* aShaderPath)
{
	auto it = myLoadedVertexShaders.find(aShaderPath);
	if (it != myLoadedVertexShaders.end())
		return &(it->second);
	return ForceLoadVertexShader(aShaderPath);
}

const ComputeShader* DX11::LoadComputeShader(const char* aShaderPath)
{
	auto it = myLoadedComputeShaders.find(aShaderPath);
	if (it != myLoadedComputeShaders.end())
		return &(it->second);
	return ForceLoadComputeShader(aShaderPath);
}

const PixelShader* DX11::ForceLoadPixelShader(const char* aShaderPath, bool addToFileWatcher)
{
	std::string csoFilepath = (Settings::CookedAssetRoot() / (std::string(aShaderPath) + ".cso")).string();

	addToFileWatcher; // needed to not cause unreferenced parameter in retail
#ifndef _RETAIL
	std::string relativePath = std::string(aShaderPath) + ".hlsl";
	std::string hlslPath = Settings::ResolveAssetPath(relativePath);
	if (!hlslPath.empty())
	{
		std::string buildInfoPath = (Settings::CookedAssetRoot() / (std::string(aShaderPath) + "_buildInfo.json")).string();
		CompileShaderIfChanged(aShaderPath, relativePath, hlslPath, buildInfoPath, csoFilepath, ShaderType::Pixel, addToFileWatcher);
	}
#endif

	PixelShader& shader = myLoadedPixelShaders[aShaderPath];

	HRESULT result;

	std::ifstream file;
	file.open(csoFilepath, std::ios::binary);
	std::string data = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
	result = DX11::Device->CreatePixelShader(data.data(), data.size(), nullptr, shader.shader.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		myLoadedPixelShaders.erase(aShaderPath);
		return nullptr;
	}
	file.close();

	return &shader;
}


const VertexShader* DX11::ForceLoadVertexShader(const char* aShaderPath, bool addToFileWatcher)
{
	VertexShader& shader = myLoadedVertexShaders[aShaderPath];

	std::string csoFilepath = (Settings::CookedAssetRoot() / (std::string(aShaderPath) + ".cso")).string();

	addToFileWatcher; // needed to not cause unreferenced parameter in retail
#ifndef _RETAIL
	std::string relativePath = std::string(aShaderPath) + ".hlsl";
	std::string hlslPath = Settings::ResolveAssetPath(relativePath);
	if (!hlslPath.empty())
	{
		std::string buildInfoPath = (Settings::CookedAssetRoot() / (std::string(aShaderPath) + "_buildInfo.json")).string();
		CompileShaderIfChanged(aShaderPath, relativePath, hlslPath, buildInfoPath, csoFilepath, ShaderType::Vertex, addToFileWatcher);
	}
#endif

	HRESULT result;

	std::ifstream file;
	file.open(csoFilepath, std::ios::binary);
	shader.data = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
	result = DX11::Device->CreateVertexShader(shader.data.data(), shader.data.size(), nullptr, shader.shader.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		myLoadedVertexShaders.erase(aShaderPath);
		return nullptr;
	}
	file.close();

	return &shader;
}

const ComputeShader* DX11::ForceLoadComputeShader(const char* aShaderPath, bool addToFileWatcher)
{
	ComputeShader& shader = myLoadedComputeShaders[aShaderPath];

	std::string csoFilepath = (Settings::CookedAssetRoot() / (std::string(aShaderPath) + ".cso")).string();

	addToFileWatcher; // needed to not cause unreferenced parameter in retail
#ifndef _RETAIL
	std::string relativePath = std::string(aShaderPath) + ".hlsl";
	std::string hlslPath = Settings::ResolveAssetPath(relativePath);
	if (!hlslPath.empty())
	{
		std::string buildInfoPath = (Settings::CookedAssetRoot() / (std::string(aShaderPath) + "_buildInfo.json")).string();
		CompileShaderIfChanged(aShaderPath, relativePath, hlslPath, buildInfoPath, csoFilepath, ShaderType::Compute, addToFileWatcher);
	}

#endif

	HRESULT result;

	std::ifstream file;
	file.open(csoFilepath, std::ios::binary);
	std::string data = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
	result = DX11::Device->CreateComputeShader(data.data(), data.size(), nullptr, shader.shader.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		myLoadedComputeShaders.erase(aShaderPath);
		return nullptr;
	}
	file.close();

	return &shader;
}
