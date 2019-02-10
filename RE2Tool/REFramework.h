#pragma once

#include <iostream>
#include <array>
#include <mutex>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include "D3D11Hook.h"
#include "DInputHook.h"
#include "WindowsMessageHook.h"
#include "FW1FontWrapper.h"
#include "imgui\\imgui.h"
#include "imgui\\imgui_impl_win32.h"
#include "imgui\\imgui_impl_dx11.h"
#include "VirtualData.h"

#define STATWND_KEY DIK_INSERT
#define STATWND_DIST 10.0f
#define MAX_ENTITY_COUNT 12

//constexpr DWORD dwStatWndKey = DIK_INSERT;
//constexpr float fOverlayDist = 10.0f;

// Structures
struct SimpleVertex
{
	DirectX::XMFLOAT3 Pos;
};

class DInputHook;

class REFramework
{
private:
	bool mInit;
	std::unique_ptr<D3D11Hook> mD3D11Hook;
	ID3D11RenderTargetView* mPtrRenderTargetView;
	ID3D11VertexShader* mPtrVertexShader;
	ID3D11PixelShader* mPtrPixelShader;
	ID3D11InputLayout* mPtrVertexLayout;
	ID3D11Buffer* mPtrVertexBuffer;
	
	// Font
	IFW1Factory* mPtrFW1Factory;
	IFW1FontWrapper* mPtrFontWrapper;

	//viewport
	D3D11_VIEWPORT mViewport;

	// UI
	bool mStatWnd;
	int mStatWndCorner;
	HWND mWnd;
	bool mInputHooked;
	std::mutex mInputMutex;
	std::array<uint8_t, 256> mLastKeys {0};
	std::unique_ptr<DInputHook> mDInputHook;
	std::unique_ptr<WindowsMessageHook> mWindowsMessageHook;

	// Virtual Data
	VirtualData<BYTE> vdIsInControl;
	VirtualData<QWORD> vdMSTime;
	VirtualData<INT> vdPlayerMaxHP;
	VirtualData<INT> vdPlayerHP;
	VirtualData<INT> vdEntityMaxHP;
	VirtualData<INT> vdEntityHP;

	HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
	bool Init();
	HRESULT InitShaders();
	void CreateRenderTarget();
	void CleanupRenderTarget();
	void OnRender();
	void OnReset();
	void DrawUI();
public:
	REFramework();
	virtual ~REFramework();

	bool OnMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnDirectInputKeys(const std::array<uint8_t, 256> &Keys);
};

extern std::unique_ptr<REFramework> gREFramework;
