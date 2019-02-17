#include <algorithm>
#include "D3D11Hook.h"

using namespace std;

static D3D11Hook* gD3D11Hook = nullptr;

D3D11Hook::~D3D11Hook()
{
	unhook();
}

bool D3D11Hook::hook()
{	
	gD3D11Hook = this;

	HWND hWnd = GetDesktopWindow();
	IDXGISwapChain* swapChain = nullptr;
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;

	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;

	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_NULL, nullptr, 0, &featureLevel, 1, D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, nullptr, &context)))
	{
		return false;
	}

	auto presentFn = (*(uintptr_t**)swapChain)[8];
	auto resizeBuffersFn = (*(uintptr_t**)swapChain)[13];

	mPresentHook = std::make_unique<FunctionHook>(presentFn, (uintptr_t)&D3D11Hook::present);
	mResizeBuffersHook = std::make_unique<FunctionHook>(resizeBuffersFn, (uintptr_t)&D3D11Hook::resizeBuffers);

	device->Release();
	context->Release();
	swapChain->Release();

	return mHooked = true;
}

bool D3D11Hook::unhook()
{
	return true;
}

HRESULT WINAPI D3D11Hook::present(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
	auto d3d11 = gD3D11Hook;

	d3d11->mSwapChain = swapChain;
	swapChain->GetDevice(__uuidof(d3d11->mDevice), (void**)&d3d11->mDevice);

	if (d3d11->mOnPresent)
	{
		d3d11->mOnPresent(*d3d11);
	}

	auto presentFn = d3d11->mPresentHook->getOriginal<decltype(D3D11Hook::present)>();

	return presentFn(swapChain, syncInterval, flags);
}

HRESULT WINAPI D3D11Hook::resizeBuffers(IDXGISwapChain* swapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	auto d3d11 = gD3D11Hook;

	if (d3d11->mOnResizeBuffers)
	{
		d3d11->mOnResizeBuffers(*d3d11);
	}

	auto resizeBuffersFn = d3d11->mResizeBuffersHook->getOriginal<decltype(D3D11Hook::resizeBuffers)>();

	return resizeBuffersFn(swapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}