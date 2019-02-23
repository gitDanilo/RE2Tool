#pragma once

#include <iostream>
#include <array>
//#include <queue>
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
#define HITMARK_KEY DIK_DELETE
#define STATWND_DIST 10.0f
#define GRAPH_REFRESH_INTERVAL 0.25
#define HITMARK_DISPLAY_INTERVAL 0.65
#define START_MAX_FPS 100.0f
#define MAX_FPS_COUNT 5
#define UPDATE_DATA_DELAY 50
#define DEFAULT_SPINCOUNT 0x000032
#define DMG_LIST_CAPACITY 12
#define HITMARK_FONT_SIZE 30.0f
#define HITMARK_X_FACTOR 0.026f
#define HITMARK_Y_FACTOR 0.065f
#define HITMARK_FONT_FACTOR 0.028f
//#define HITMARK_COLOR 0xFF1EFB52
#define HITMARK_COLOR 0xFFFFFFFF

//typedef int (WINAPI MESSAGEBOXA)(HWND, LPCSTR, LPCSTR, UINT);
typedef void (WINAPI *ONDMGFUNCTION)(QWORD, QWORD, QWORD);

// Structures
struct SimpleVertex
{
	DirectX::XMFLOAT3 Pos;
};

typedef struct _HITM_INFO
{
	FLOAT X;
	FLOAT Y;
	FLOAT fFontSize;
	DWORD dwColor;
} HITM_INFO, *PHITM_INFO;

static BYTE AOB_JUMP[] =
{
	// mov rax, x64_addr
	0x48, 0xB8, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
	// jmp rax
	0xFF, 0xE0
};

static const BYTE AOB_ORIGINAL[] =
{
	0x8B, 0x43, 0x7C, 0x89, 0x46, 0x7C, 0x48, 0x8B, 0x47, 0x50
};

static const BYTE AOB_DETOUR[] =
{
	0x8B, 0x43, 0x7C, 0x89, 0x46, 0x7C, 0x48, 0x8B, 0x47, 0x50
};

static const BYTE AOB_DMG_HANDLE[] =
{
	0x8B, 0x43, 0x7C, 0x89, 0x46, 0x7C, 0x48, 0x8B, 0x47, 0x50
};

namespace ModuleID
{
	enum ID: DWORD
	{
		base
	};
}

#define PATTERNLIST_SIZE 4
static AOB_PATTERN PatternList[PATTERNLIST_SIZE] =
{
	{ModuleID::base, 0, 0, const_cast<BYTE*>(AOB_DMG_HANDLE), sizeof(AOB_DMG_HANDLE), nullptr, 0}
};

namespace SigID
{
	enum ID: DWORD
	{
		dmg_handle
	};
}

// Commented out in original ImGui code
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

	FW1_FONTWRAPPERCREATEPARAMS mFontParams;

	//std::unique_ptr<FunctionHook> mRE2DmgHandle {};

	// UI
	CRITICAL_SECTION mCSInput;
	HWND mWnd;
	bool mStatWnd;
	bool mHitmark;
	bool bResetTimer;
	int mStatWndCorner;
	bool mInputHooked;
	std::array<uint8_t, 256> mLastKeys;
	std::unique_ptr<DInputHook> mDInputHook;
	std::unique_ptr<WindowsMessageHook> mWindowsMessageHook;

	// Virtual Data
	CRITICAL_SECTION mCSData;
	RE_DATA mREData;
	VirtualData<BYTE> mVDIsInControl;
	VirtualData<QWORD> mVDActiveTime;
	VirtualData<QWORD> mVDCutsceneTime;
	VirtualData<QWORD> mVDPausedTime;
	VirtualData<INT> mVDPlayerMaxHP;
	VirtualData<INT> mVDPlayerHP;
	std::vector<VirtualData<INT>> mEntityMaxHPList;
	std::vector<VirtualData<INT>> mEntityHPList;
	
	LPVOID pExecMem;

	// Hitmark
	HITM_INFO mHitmInf;
	HANDLE mUpdateDataThreadHnd;
	ONDMGFUNCTION mPtrDamageFunction;
	std::vector<INT> mDmgList;
	INT mLastDmg;

	static void WINAPI OnDamageReceived(QWORD qwP1, QWORD qwP2, QWORD qwP3);

	// Thread Starter
	static DWORD WINAPI StartUpdateDataThread(LPVOID lpParam);
	DWORD UpdateData();
	
	HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
	bool Init();
	HRESULT InitShaders();
	void CreateRenderTarget();
	void CleanupRenderTarget();
	void OnRender();
	void OnReset();
	void DrawUI();
	void DrawHitmark(ID3D11DeviceContext* &pContext);
public:
	REFramework();
	virtual ~REFramework();

	bool OnMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnDirectInputKeys(const std::array<uint8_t, 256> &Keys);
};

extern std::unique_ptr<REFramework> gREFramework;
