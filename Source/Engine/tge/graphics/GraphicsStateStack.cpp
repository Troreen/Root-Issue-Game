#include "stdafx.h"
#include "GraphicsStateStack.h"

#include <tge/graphics/DX11.h>
#include <tge/shaders/ShaderCommon.h>
#include <tge/texture/TextureManager.h>
#include <tge/util/StringCast.h>

using namespace Tga;

struct ShaderSettingsConstantBufferData
{
	Vector4f customShaderParameters;
	float alphaTestThreshold;
	float unused0;
	float unused1;
	float unused2;
};

struct FrameConstantBufferData
{
	Vector2f resolution;
	float totalTime;
	float deltaTime;
};

struct CameraConstantBufferData
{
	Matrix4x4f worldToCamera;
	Matrix4x4f cameraToWorld;
	Matrix4x4f worldToProjection;
	Matrix4x4f projectionToWorld;
	float nearPlane;
	float farPlane;
	float unused0;
	float unused1;
};

struct LightConstantBufferData
{
	struct PointLightData
	{
		Vector4f position;
		Vector4f color;
		float range;
		float lightRadius;
		float padding0;
		float padding1;
	} pointLights[NUMBER_OF_LIGHTS_ALLOWED];

	unsigned int numberOfLights;
	float directionalLightSoftness;
	float garbage0;
	float garbage1;

	Vector4f ambientLightColor;
	Vector4f directionalLightColor;
	Matrix4x4f directionalLightTransform;
};

bool GraphicsStateStack::Init()
{
	if (!CreateBlendStates())
		return false;
	if (!CreateDepthStencilStates())
		return false;
	if (!CreateRasterizerStates())
		return false;
	if (!CreateSamplers())
		return false;
	if (!CreateConstantBuffers())
		return false;

	myRenderStateStack.resize(1);
	
	{
		TextureManager& textureManager = Engine::GetInstance()->GetTextureManager();
		myDefaultCubemap = textureManager.GetTexture("Textures/whiteCubeMap.dds", TextureSrgbMode::None);
		myDefaultHorizonCubemap = textureManager.GetTexture("Textures/horizonCubeMap.dds", TextureSrgbMode::None);
	}

	SetAllStatesToDefault(true);

	return true;
}

void GraphicsStateStack::BeginFrame()
{
	assert(myRenderStateStack.size() == 1);

	// Frame buffer is only set once per frame
	{
		D3D11_MAPPED_SUBRESOURCE mappedResourceCommon;
		FrameConstantBufferData* dataPtrCommon;
		HRESULT result = DX11::Context->Map(myFrameConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResourceCommon);
		if (FAILED(result))
		{
			INFO_PRINT("Error in rendering!");
			return;
		}

		dataPtrCommon = (FrameConstantBufferData*)mappedResourceCommon.pData;
		dataPtrCommon->resolution = Vector2f(static_cast<float>(DX11::GetResolution().x), static_cast<float>(DX11::GetResolution().y));
		dataPtrCommon->totalTime = Engine::GetInstance()->GetTotalTime();
		dataPtrCommon->deltaTime = Engine::GetInstance()->GetDeltaTime();

		DX11::Context->Unmap(myFrameConstantBuffer.Get(), 0);
	}

	// reset everything else to default values
	SetAllStatesToDefault(true);
}

void GraphicsStateStack::Push()
{
	myRenderStateStack.push_back(myRenderStateStack.back());
}

void GraphicsStateStack::Pop()
{
	assert(myRenderStateStack.size() > 1);

	if (myRenderStateStack.size() == 1)
		return;

	myRenderStateStack.pop_back();
}


void GraphicsStateStack::SetAllStatesToDefault(bool fullReset)
{
	RenderState renderState = {};
	renderState.blendState = BlendState::AlphaBlend;
	renderState.alphaTestThreshold = 0.01f;
	renderState.rasterizerState = RasterizerState::NoFaceCulling;
	renderState.depthStencilState = DepthStencilState::WriteLess;
	renderState.samplerAddressMode = SamplerAddressMode::Wrap;
	renderState.samplerFilter = SamplerFilter::Trilinear;

	renderState.camera = {};
	renderState.camera.SetOrtographicProjection(0.0f, (float)DX11::GetResolution().x, 0.0f, (float)DX11::GetResolution().y, -1.0f, 1.f);
	renderState.camera.SetTransform({});

	renderState.ambientLight = {};

	// clear out versions if doing a full reset.
	if (!fullReset)
	{
		renderState.shaderDataVersion = myRenderStateStack.back().shaderDataVersion;
		renderState.lightDataVersion = myRenderStateStack.back().lightDataVersion;
		renderState.cameraDataVersion = myRenderStateStack.back().cameraDataVersion;
	}
	else
	{
		myLatestCameraDataVersion = 0;
		myLatestLightDataVersion = 0;
		myLatestShaderDataVersion = 0;
	}

	myRenderStateStack.back() = renderState;
	UpdateGpuStates(fullReset);
}

void GraphicsStateStack::SetBlendState(BlendState aBlendState)
{
	myRenderStateStack.back().blendState = aBlendState;
}

void GraphicsStateStack::SetDepthStencilState(DepthStencilState aDepthStencilState)
{
	myRenderStateStack.back().depthStencilState = aDepthStencilState;
}

void GraphicsStateStack::SetRasterizerState(RasterizerState aRasterizerState)
{
	myRenderStateStack.back().rasterizerState = aRasterizerState;
}

void GraphicsStateStack::SetSamplerState(SamplerFilter aFilter, SamplerAddressMode aAddressMode)
{
	myRenderStateStack.back().samplerFilter = aFilter;
	myRenderStateStack.back().samplerAddressMode = aAddressMode;
}

BlendState GraphicsStateStack::GetBlendState() const
{
	return myRenderStateStack.back().blendState;
}

DepthStencilState GraphicsStateStack::GetDepthStencilState() const
{
	return myRenderStateStack.back().depthStencilState;
}

RasterizerState GraphicsStateStack::GetRasterizerState() const
{
	return myRenderStateStack.back().rasterizerState;
}

SamplerFilter GraphicsStateStack::GetSamplerFilter() const
{
	return myRenderStateStack.back().samplerFilter;
}

SamplerAddressMode GraphicsStateStack::GetSamplerAdressMode() const
{
	return myRenderStateStack.back().samplerAddressMode;
}

void GraphicsStateStack::SetAlphaTestThreshold(float aAlphaTestThreshold)
{
	myRenderStateStack.back().shaderDataVersion = ++myLatestShaderDataVersion;
	myRenderStateStack.back().alphaTestThreshold = aAlphaTestThreshold;
}

float GraphicsStateStack::GetAlphaTestThreshold() const
{
	return myRenderStateStack.back().alphaTestThreshold;
}

void GraphicsStateStack::SetCustomShaderParameters(Vector4f aCustomShaderParameters)
{
	myRenderStateStack.back().shaderDataVersion = ++myLatestShaderDataVersion;
	myRenderStateStack.back().customShaderParameters = aCustomShaderParameters;
}

Vector4f GraphicsStateStack::GetCustomShaderParameters() const
{
	return myRenderStateStack.back().customShaderParameters;
}

void GraphicsStateStack::SetCamera(const Camera& camera)
{
	myRenderStateStack.back().cameraDataVersion = ++myLatestCameraDataVersion;
	myRenderStateStack.back().camera = camera;
}

void GraphicsStateStack::SetDefaultCamera()
{
	myRenderStateStack.back().cameraDataVersion = ++myLatestCameraDataVersion;
	myRenderStateStack.back().camera = {};
	myRenderStateStack.back().camera.SetOrtographicProjection(0.0f, (float)DX11::GetResolution().x, 0.0f, (float)DX11::GetResolution().y, -1.0f, 1.f);
	myRenderStateStack.back().camera.SetTransform({});
}

const Camera& GraphicsStateStack::GetCamera() const
{
	return myRenderStateStack.back().camera;
}

size_t GraphicsStateStack::GetPointLightCount() const 
{
	return myRenderStateStack.back().pointLightCount;
}

void GraphicsStateStack::ClearPointLights()
{
	myRenderStateStack.back().lightDataVersion = ++myLatestLightDataVersion;
	myRenderStateStack.back().pointLightCount = 0;
}

void GraphicsStateStack::AddPointLight(const PointLight& aPointLight) 
{
	myRenderStateStack.back().lightDataVersion = ++myLatestLightDataVersion;

	RenderState& state = myRenderStateStack.back();

	assert(state.pointLightCount < NUMBER_OF_LIGHTS_ALLOWED);

	if (state.pointLightCount >= NUMBER_OF_LIGHTS_ALLOWED)
		return;

	state.pointLights[state.pointLightCount] = aPointLight;
	state.pointLightCount++;
}

const PointLight* GraphicsStateStack::GetPointLights() const
{
	return myRenderStateStack.back().pointLights;
}

void GraphicsStateStack::SetDirectionalLight(DirectionalLight light)
{
	myRenderStateStack.back().lightDataVersion = ++myLatestLightDataVersion;
	myRenderStateStack.back().directionalLight = light;
}

const DirectionalLight& GraphicsStateStack::GetDirectionalLight()
{
	return myRenderStateStack.back().directionalLight;
}

void GraphicsStateStack::SetAmbientLight(AmbientLight light)
{
	myRenderStateStack.back().lightDataVersion = ++myLatestLightDataVersion;
	myRenderStateStack.back().ambientLight = light;
}

const AmbientLight& GraphicsStateStack::GetAmbientLight()
{
	return myRenderStateStack.back().ambientLight;
}

void GraphicsStateStack::SetTransform(Matrix4x4f transform)
{
	myRenderStateStack.back().transform = transform;
}

void GraphicsStateStack::ApplyTransform(Matrix4x4f transform)
{
	myRenderStateStack.back().transform = transform * myRenderStateStack.back().transform;
}

void GraphicsStateStack::Translate(Vector3f position)
{
	myRenderStateStack.back().transform = Matrix4x4f::CreateFromTranslation(position) * myRenderStateStack.back().transform;
}

void GraphicsStateStack::Scale(float scale)
{
	myRenderStateStack.back().transform = Matrix4x4f::CreateFromScale(scale) * myRenderStateStack.back().transform;
}

void GraphicsStateStack::Scale(Vector3f scale)
{
	myRenderStateStack.back().transform = Matrix4x4f::CreateFromScale(scale) * myRenderStateStack.back().transform;
}

void GraphicsStateStack::Rotate(Vector3f rotation)
{
	myRenderStateStack.back().transform = Matrix4x4f::CreateFromRollPitchYaw(rotation) * myRenderStateStack.back().transform;
}

void GraphicsStateStack::Rotate(Quatf rotation)
{
	myRenderStateStack.back().transform = Matrix4x4f::CreateFromRotation(rotation) * myRenderStateStack.back().transform;
}

const Matrix4x4f& GraphicsStateStack::GetTransform() const
{
	return myRenderStateStack.back().transform;
}

Vector3f GraphicsStateStack::GetPosition() const
{
	return myRenderStateStack.back().transform.GetPosition();
}

void GraphicsStateStack::UpdateGpuStates(bool fullReset)
{
	RenderState& state = myRenderStateStack.back();

	if (fullReset || state.blendState != myGpuRenderState.blendState)
	{
		DX11::Context->OMSetBlendState(myBlendStates[(int)state.blendState].Get(), nullptr, 0xffffffff);
	}

	if (fullReset || state.depthStencilState != myGpuRenderState.depthStencilState)
	{
		DX11::Context->OMSetDepthStencilState(myDepthStencilStates[(int)state.depthStencilState].Get(), 0);
	}

	if (fullReset || state.rasterizerState != myGpuRenderState.rasterizerState)
	{
		DX11::Context->RSSetState(myRasterizerStates[(int)state.rasterizerState].Get());
	}

	if (fullReset || state.samplerFilter != myGpuRenderState.samplerFilter || state.samplerAddressMode != myGpuRenderState.samplerAddressMode)
	{
		DX11::Context->PSSetSamplers(0, 1, mySamplerStates[(int)state.samplerFilter][(int)state.samplerAddressMode].GetAddressOf());
	}

	if (fullReset || state.shaderDataVersion != myGpuRenderState.shaderDataVersion)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		ShaderSettingsConstantBufferData* dataPtr;
		HRESULT result = DX11::Context->Map(myShaderSettingsConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (FAILED(result))
		{
			INFO_PRINT("Error in rendering!");
			return;
		}

		dataPtr = (ShaderSettingsConstantBufferData*)mappedResource.pData;
		dataPtr->alphaTestThreshold = state.alphaTestThreshold;
		dataPtr->customShaderParameters = state.customShaderParameters;

		DX11::Context->Unmap(myShaderSettingsConstantBuffer.Get(), 0);
	}

	if (fullReset || state.cameraDataVersion != myGpuRenderState.cameraDataVersion)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResourceCommon;
		CameraConstantBufferData* dataPtrCommon;
		HRESULT result = DX11::Context->Map(myCameraConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResourceCommon);
		if (FAILED(result))
		{
			INFO_PRINT("Error in rendering!");
			return;
		}

		dataPtrCommon = (CameraConstantBufferData*)mappedResourceCommon.pData;
		dataPtrCommon->cameraToWorld = state.camera.GetTransform();
		dataPtrCommon->worldToCamera = Matrix4x4f::GetFastInverse(state.camera.GetTransform());
		dataPtrCommon->worldToProjection = state.camera.GetProjection();
		dataPtrCommon->projectionToWorld = state.camera.GetProjection().GetInverse();

		float camNearPlane = 0;
		float camFarPlane = 0;
		state.camera.GetProjectionPlanes(camNearPlane, camFarPlane);

		dataPtrCommon->nearPlane = camNearPlane;
		dataPtrCommon->farPlane = camFarPlane;

		DX11::Context->Unmap(myCameraConstantBuffer.Get(), 0);
	}

	if (fullReset || state.lightDataVersion != myGpuRenderState.lightDataVersion)
	{
#ifdef USE_LIGHTS

		D3D11_MAPPED_SUBRESOURCE mappedResourceLight;
		LightConstantBufferData* dataPtrLights;

		HRESULT result = DX11::Context->Map(myLightConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResourceLight);
		if (FAILED(result))
		{
			INFO_PRINT("Error in rendering! Could not map lights");
			return;
		}

		size_t pointLightCount = state.pointLightCount;
		const PointLight* pointLights = state.pointLights;
		dataPtrLights = (LightConstantBufferData*)mappedResourceLight.pData;
		*dataPtrLights = {};

		dataPtrLights->numberOfLights = static_cast<unsigned int>(pointLightCount);

		for (int i = 0; i < pointLightCount; i++)
		{
			const PointLight& pointLight = pointLights[i];
			dataPtrLights->pointLights[i].color = pointLight.color.AsLinearVec4();
			dataPtrLights->pointLights[i].position = pointLight.position;
			dataPtrLights->pointLights[i].range = pointLight.range;
			dataPtrLights->pointLights[i].lightRadius = pointLight.radius;

		}

		const DirectionalLight& directionalLight = state.directionalLight;
		const AmbientLight& ambientLight = state.ambientLight;
		dataPtrLights->directionalLightTransform = directionalLight.transform;
		dataPtrLights->directionalLightColor = directionalLight.color.AsLinearVec4();
		dataPtrLights->directionalLightSoftness = directionalLight.softness;
		dataPtrLights->ambientLightColor = ambientLight.color.AsLinearVec4();

		DX11::Context->Unmap(myLightConstantBuffer.Get(), 0);
		
		ID3D11ShaderResourceView* cubemap = nullptr;
		switch (ambientLight.type)
		{
		case AmbientLightType::Custom:
			if (ambientLight.cubemap)
				cubemap = ambientLight.cubemap->GetShaderResourceView();
			break;
		case AmbientLightType::Uniform:
			if (myDefaultCubemap)
				cubemap = myDefaultCubemap->GetShaderResourceView();
			break;
		case AmbientLightType::UniformAboveHorizon:
			if (myDefaultHorizonCubemap)
				cubemap = myDefaultHorizonCubemap->GetShaderResourceView();
			break;
		}
		DX11::Context->PSSetShaderResources(0, 1, &cubemap);

#endif
	}

	if (fullReset)
	{
		DX11::Context->VSSetConstantBuffers((int)ConstantBufferSlot::Frame, 1, myFrameConstantBuffer.GetAddressOf());
		DX11::Context->PSSetConstantBuffers((int)ConstantBufferSlot::Frame, 1, myFrameConstantBuffer.GetAddressOf());

		DX11::Context->VSSetConstantBuffers((int)ConstantBufferSlot::Camera, 1, myCameraConstantBuffer.GetAddressOf());
		DX11::Context->PSSetConstantBuffers((int)ConstantBufferSlot::Camera, 1, myCameraConstantBuffer.GetAddressOf());

		DX11::Context->VSSetConstantBuffers((int)ConstantBufferSlot::ShaderSettings, 1, myShaderSettingsConstantBuffer.GetAddressOf());
		DX11::Context->PSSetConstantBuffers((int)ConstantBufferSlot::ShaderSettings, 1, myShaderSettingsConstantBuffer.GetAddressOf());

		DX11::Context->VSSetConstantBuffers((int)ConstantBufferSlot::Light, 1, myLightConstantBuffer.GetAddressOf());
		DX11::Context->PSSetConstantBuffers((int)ConstantBufferSlot::Light, 1, myLightConstantBuffer.GetAddressOf());
	}

	myGpuRenderState = state;
}

bool GraphicsStateStack::CreateBlendStates()
{
	HRESULT result = S_OK;
	D3D11_BLEND_DESC blendStateDescription = {};

	blendStateDescription.RenderTarget[0].BlendEnable = FALSE;
	blendStateDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
	blendStateDescription.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	blendStateDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	result = DX11::Device->CreateBlendState(&blendStateDescription, myBlendStates[(int)BlendState::Disabled].ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	blendStateDescription = {};
	blendStateDescription.RenderTarget[0].BlendEnable = TRUE;
	blendStateDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDescription.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendStateDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blendStateDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
	blendStateDescription.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	result = DX11::Device->CreateBlendState(&blendStateDescription, myBlendStates[(int)BlendState::AlphaBlend].ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	blendStateDescription = {};
	blendStateDescription.RenderTarget[0].BlendEnable = TRUE;
	blendStateDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDescription.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendStateDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendStateDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blendStateDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
	blendStateDescription.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	result = DX11::Device->CreateBlendState(&blendStateDescription, myBlendStates[(int)BlendState::AdditiveBlend].ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

bool GraphicsStateStack::CreateDepthStencilStates()
{
	HRESULT result = S_OK;

	D3D11_DEPTH_STENCIL_DESC writeLessDesc = {};
	writeLessDesc.DepthEnable = true;
	writeLessDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	writeLessDesc.DepthFunc = D3D11_COMPARISON_LESS;
	writeLessDesc.StencilEnable = false;

	D3D11_DEPTH_STENCIL_DESC writeLessOrEqualDesc = {};
	writeLessOrEqualDesc.DepthEnable = true;
	writeLessOrEqualDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	writeLessOrEqualDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	writeLessOrEqualDesc.StencilEnable = false;


	D3D11_DEPTH_STENCIL_DESC readOnlyLessDesc = {};
	readOnlyLessDesc.DepthEnable = true;
	readOnlyLessDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	readOnlyLessDesc.DepthFunc = D3D11_COMPARISON_LESS;
	readOnlyLessDesc.StencilEnable = false;

	D3D11_DEPTH_STENCIL_DESC readOnlyLessOrEqualDesc = {};
	readOnlyLessOrEqualDesc.DepthEnable = true;
	readOnlyLessOrEqualDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	readOnlyLessOrEqualDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	readOnlyLessOrEqualDesc.StencilEnable = false;

	result = DX11::Device->CreateDepthStencilState(&writeLessDesc, &myDepthStencilStates[(int)DepthStencilState::WriteLess]);
	if (FAILED(result))
		return false;

	result = DX11::Device->CreateDepthStencilState(&writeLessOrEqualDesc, &myDepthStencilStates[(int)DepthStencilState::WriteLessOrEqual]);
	if (FAILED(result))
		return false;

	result = DX11::Device->CreateDepthStencilState(&readOnlyLessDesc, &myDepthStencilStates[(int)DepthStencilState::ReadOnlyLess]);
	if (FAILED(result))
		return false;

	result = DX11::Device->CreateDepthStencilState(&readOnlyLessOrEqualDesc, &myDepthStencilStates[(int)DepthStencilState::ReadOnlyLessOrEqual]);
	if (FAILED(result))
		return false;

	return true;
}

bool GraphicsStateStack::CreateRasterizerStates()
{
	HRESULT result = S_OK;

	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthClipEnable = true;

	result = DX11::Device->CreateRasterizerState(&rasterizerDesc, &myRasterizerStates[(int)RasterizerState::Wireframe]);
	if (FAILED(result))
		return false;

	rasterizerDesc.CullMode = D3D11_CULL_NONE;

	result = DX11::Device->CreateRasterizerState(&rasterizerDesc, &myRasterizerStates[(int)RasterizerState::WireframeNoCulling]);
	if (FAILED(result))
		return false;

	rasterizerDesc = {};
	rasterizerDesc.AntialiasedLineEnable = false;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.FrontCounterClockwise = false;
	rasterizerDesc.MultisampleEnable = true;
	rasterizerDesc.ScissorEnable = false;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;

	result = DX11::Device->CreateRasterizerState(&rasterizerDesc, &myRasterizerStates[(int)RasterizerState::NoFaceCulling]);
	if (FAILED(result))
		return false;

	myRasterizerStates[(int)RasterizerState::BackfaceCulling] = nullptr;

	return true;
}

bool GraphicsStateStack::CreateSamplers()
{
	HRESULT result = S_OK;
	D3D11_SAMPLER_DESC samplerDesc = {};

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	result = DX11::Device->CreateSamplerState(&samplerDesc, mySamplerStates[(int)SamplerFilter::Point][(int)SamplerAddressMode::Clamp].ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	result = DX11::Device->CreateSamplerState(&samplerDesc, mySamplerStates[(int)SamplerFilter::Point][(int)SamplerAddressMode::Wrap].ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	result = DX11::Device->CreateSamplerState(&samplerDesc, mySamplerStates[(int)SamplerFilter::Point][(int)SamplerAddressMode::Mirror].ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	result = DX11::Device->CreateSamplerState(&samplerDesc, mySamplerStates[(int)SamplerFilter::Bilinear][(int)SamplerAddressMode::Clamp].ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	result = DX11::Device->CreateSamplerState(&samplerDesc, mySamplerStates[(int)SamplerFilter::Bilinear][(int)SamplerAddressMode::Wrap].ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	result = DX11::Device->CreateSamplerState(&samplerDesc, mySamplerStates[(int)SamplerFilter::Bilinear][(int)SamplerAddressMode::Mirror].ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	result = DX11::Device->CreateSamplerState(&samplerDesc, mySamplerStates[(int)SamplerFilter::Trilinear][(int)SamplerAddressMode::Clamp].ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	result = DX11::Device->CreateSamplerState(&samplerDesc, mySamplerStates[(int)SamplerFilter::Trilinear][(int)SamplerAddressMode::Wrap].ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	result = DX11::Device->CreateSamplerState(&samplerDesc, mySamplerStates[(int)SamplerFilter::Trilinear][(int)SamplerAddressMode::Mirror].ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

static bool CreateBuffer(uint32_t aSize, ComPtr<ID3D11Buffer>& aBuffer)
{
	HRESULT result = S_OK;

	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = aSize;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	result = DX11::Device->CreateBuffer(&bufferDesc, NULL, aBuffer.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}	

	return true;
}

bool GraphicsStateStack::CreateConstantBuffers()
{
	if (!CreateBuffer(sizeof(ShaderSettingsConstantBufferData), myShaderSettingsConstantBuffer))
	{
		return false;
	}
	if (!CreateBuffer(sizeof(FrameConstantBufferData), myFrameConstantBuffer))
	{
		return false;
	}
	if (!CreateBuffer(sizeof(LightConstantBufferData), myLightConstantBuffer))
	{
		return false;
	}
	if (!CreateBuffer(sizeof(CameraConstantBufferData), myCameraConstantBuffer))
	{
		return false;
	}
	return true;
}