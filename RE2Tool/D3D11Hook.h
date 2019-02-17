#pragma once

#include <iostream>
#include <functional>
#include <d3d11.h>
#include <dxgi.h>
#include "FunctionHook.h"

class D3D11Hook
{
public:
	typedef std::function<void(D3D11Hook&)> OnPresentFn;
	typedef std::function<void(D3D11Hook&)> OnResizeBuffersFn;

	D3D11Hook() = default;
	virtual ~D3D11Hook();

	bool hook();
	bool unhook();

	void OnPresent(OnPresentFn fn)
	{
		mOnPresent = fn;
	}
	void OnResizeBuffers(OnResizeBuffersFn fn)
	{
		mOnResizeBuffers = fn;
	}

	ID3D11Device* getDevice()
	{
		return mDevice;
	}
	IDXGISwapChain* getSwapChain()
	{
		return mSwapChain;
	}

protected:
	ID3D11Device* mDevice {nullptr};
	IDXGISwapChain* mSwapChain {nullptr};
	bool mHooked {false};

	std::unique_ptr<FunctionHook> mPresentHook {};
	std::unique_ptr<FunctionHook> mResizeBuffersHook {};
	OnPresentFn mOnPresent {nullptr};
	OnResizeBuffersFn mOnResizeBuffers {nullptr};

	static HRESULT WINAPI Present(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
	static HRESULT WINAPI ResizeBuffers(IDXGISwapChain* swapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
};