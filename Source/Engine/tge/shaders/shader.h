#pragma once
#include <tge/math/matrix4x4.h>
#include <tge/render/RenderObject.h>
#include <d3dcommon.h>
#include <d3d11.h>
#include <tge/EngineDefines.h>
#include <wrl/client.h>
#include <functional>

using Microsoft::WRL::ComPtr;

namespace Tga
{
    struct VertexShader;
    struct PixelShader;

    class GraphicsEngine;
    class Engine;
    class Shader
    {
    public:
        Shader();
        virtual ~Shader();
        virtual bool Init(){ return false; }

		typedef std::function<void(const std::string& aBlob)> callback_layout;
		bool CreateShaders(const char* aVertex, const char* aPixel, callback_layout aLayout = nullptr);
        bool PrepareRender() const;

    protected:
		virtual bool CreateInputLayout(const std::string& aVS) { aVS; return false; }
        const VertexShader* myVertexShader;
        const PixelShader* myPixelShader;
        ComPtr<ID3D11InputLayout> myLayout;            // the pointer to the input layout

		bool myIsReadyToRender;
		int myRandomSeed;

        std::string myVertexShaderFile;
        std::string myPixelShaderFile;
    };
}