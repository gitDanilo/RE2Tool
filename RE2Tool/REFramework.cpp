#include "REFramework.h"

std::unique_ptr<REFramework> gREFramework;

REFramework::REFramework()
{
	mInit = false;
	mSetup = false;
	mStatWnd = false;
	mWnd = 0;
	mInputHooked = false;
	mStatWndCorner = 0;
	mPtrFW1Factory = nullptr;
	//mExecMem = nullptr;
	mLastDmg = 0;
	mLastSize = 0;
	//mPtrDamageFunction = nullptr;

	ZeroMemory(&mFontParams, sizeof(FW1_FONTWRAPPERCREATEPARAMS));
	mFontParams.GlyphSheetWidth = 512;
	mFontParams.GlyphSheetHeight = 512;
	mFontParams.MaxGlyphCountPerSheet = 2048;
	mFontParams.SheetMipLevels = 1;
	mFontParams.AnisotropicFiltering = FALSE;
	mFontParams.MaxGlyphWidth = 384;
	mFontParams.MaxGlyphHeight = 384;
	mFontParams.DisableGeometryShader = FALSE;
	mFontParams.VertexBufferSize = 0;
	//mFontParams.DefaultFontParams.pszFontFamily = L"Consolas";
	mFontParams.DefaultFontParams.pszFontFamily = L"BankGothic Lt BT";
	mFontParams.DefaultFontParams.FontWeight = DWRITE_FONT_WEIGHT_BOLD;
	mFontParams.DefaultFontParams.FontStyle = DWRITE_FONT_STYLE_NORMAL;
	mFontParams.DefaultFontParams.FontStretch = DWRITE_FONT_STRETCH_NORMAL;
	mFontParams.DefaultFontParams.pszLocale = L"";

	ZeroMemory(&mREData, sizeof(RE_DATA));

	mDmgList.reserve(DMG_LIST_CAPACITY);

	if (!InitializeCriticalSectionAndSpinCount(&mCSInput, DEFAULT_SPINCOUNT))
	{
		MessageBox(0, "Failed to initialize critical section.", "RE2Tool", MB_ICONERROR);
		return;
	}

	if (!InitializeCriticalSectionAndSpinCount(&mCSData, DEFAULT_SPINCOUNT))
	{
		MessageBox(0, "Failed to initialize critical section.", "RE2Tool", MB_ICONERROR);
		return;
	}

	// Initialize virtual memory pointers
	HMODULE BaseModuleAddr = GetModuleHandle(0);
	
	mVDIsInControl.SetDataPtr (reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x707C0C0, {0x140, 0xF8, 0x10, 0x28, 0x130});
	mVDActiveTime.SetDataPtr  (reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x70B0910, {0x2E0, 0x218, 0x610, 0x710, 0x60, 0x18});
	mVDCutsceneTime.SetDataPtr(reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x70B0910, {0x2E0, 0x218, 0x610, 0x710, 0x60, 0x20});
	mVDPausedTime.SetDataPtr  (reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x70B0910, {0x2E0, 0x218, 0x610, 0x710, 0x60, 0x30});
	mVDPlayerMaxHP.SetDataPtr (reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x70B0840, {0x50, 0x20, 0x54});
	mVDPlayerHP.SetDataPtr    (reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x70B0840, {0x50, 0x20, 0x58});

	mEntityMaxHPList.reserve(MAX_ENTITY_COUNT);
	mEntityHPList.reserve(MAX_ENTITY_COUNT);

	for (DWORD i = 0; i < MAX_ENTITY_COUNT; ++i)
	{
		mEntityMaxHPList.push_back(VirtualData<INT>(reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x707C528, {0x260 + (i * 0x8), 0x70, 0x18, 0x288, 0x54}));
		mEntityHPList.push_back   (VirtualData<INT>(reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x707C528, {0x260 + (i * 0x8), 0x70, 0x18, 0x288, 0x58}));
	}
	
	// hook dmg function
	std::cout << "Hooking RE2 OnDamageReceived()..." << std::endl;
	
	BYTE* ModuleBaseAddr = nullptr;
	BYTE* AOBAddr = /*reinterpret_cast<BYTE*>(0x14ACD6FF0)*/nullptr;
	INT iOffset = 0;
	DWORD dwModuleSize = 0;

	if (memutil::GetModuleInfo(PROCESS_NAME, ModuleBaseAddr, dwModuleSize))
	{
		AOBAddr = memutil::AOBScan(ModuleBaseAddr, dwModuleSize, PatternList[SigID::dmg_handle].aobPattern, PatternList[SigID::dmg_handle].dwPatternSize);
		AOBAddr += PatternList[SigID::dmg_handle].dwOffset;

		iOffset = *reinterpret_cast<INT*>(AOBAddr + 0x1); // relative jump offset
		AOBAddr += 5; // jump address
		AOBAddr += iOffset; // final address

		iOffset = *reinterpret_cast<INT*>(AOBAddr + 0x1); // relative jump offset
		AOBAddr += 5; // jump address
		AOBAddr += iOffset; // final address
	}

	if (AOBAddr == nullptr)
	{
		MessageBox(0, "Failed to find the signature of OnDamageReceived().", "RE2Tool", MB_ICONERROR);
		return;
	}

	mOnDmgRecvHook = std::make_unique<FunctionHook>(AOBAddr, &REFramework::OnDamageReceived);

	mD3D11Hook = std::make_unique<D3D11Hook>();
	mD3D11Hook->OnPresent([this](D3D11Hook& hook)
	{
		OnRender();
	});
	mD3D11Hook->OnResizeBuffers([this](D3D11Hook& hook)
	{
		OnReset();
	});

	if (mD3D11Hook->hook() == false)
	{
		MessageBox(0, "Failed to hook D3D11.", "RE2Tool", MB_ICONERROR);
		return;
	}

	mInit = true;
}

REFramework::~REFramework()
{
	if (this->mUpdateDataThreadHnd)
		TerminateThread(this->mUpdateDataThreadHnd, 0);

	DeleteCriticalSection(&mCSInput);
	DeleteCriticalSection(&mCSData);

	mOnDmgRecvHook.reset();
}

bool REFramework::OnMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (!mSetup)
	{
		return true;
	}

	if (mStatWnd && ImGui_ImplWin32_WndProcHandler(wnd, message, wParam, lParam) != 0)
	{
		// If the user is interacting with the UI we block the message from going to the game.
		auto& io = ImGui::GetIO();

		if (io.WantCaptureMouse || io.WantCaptureKeyboard || io.WantTextInput)
		{
			return false;
		}
	}

	return true;
}

void REFramework::OnDirectInputKeys(const std::array<uint8_t, 256> &Keys)
{
	if (Keys[STATWND_KEY] && mLastKeys[STATWND_KEY] == 0)
	{
		EnterCriticalSection(&mCSInput);

		++mStatWndCorner;
		if (mStatWndCorner > 3)
		{
			mStatWndCorner = -1;
			mStatWnd = false;
		}
		else
			mStatWnd = true;

		LeaveCriticalSection(&mCSInput);
	}
	else if (Keys[HITMARK_KEY] && mLastKeys[HITMARK_KEY] == 0)
	{
		EnterCriticalSection(&mCSInput);

		mHitmark = !mHitmark;
		if (mHitmark)
			mResetTimer = true;
		mDmgList.clear();
		mLastSize = 0;

		LeaveCriticalSection(&mCSInput);
	}

	mLastKeys = Keys;
}

bool REFramework::IsInit()
{
	return mInit;
}

DWORD WINAPI REFramework::StartUpdateDataThread(LPVOID lpParam)
{
	REFramework* pREFramework = static_cast<REFramework*>(lpParam);
	return pREFramework->UpdateData();
}

DWORD REFramework::UpdateData()
{
	do
	{
		static bool bData;
		static DWORD i;
		static RE_DATA TmpData;

		ZeroMemory(&TmpData, sizeof(RE_DATA));

		mVDIsInControl.GetData(TmpData.bIsInControl);
		mVDPlayerMaxHP.GetData(TmpData.iPlayerMaxHP);

		if (TmpData.bIsInControl == 1 && TmpData.iPlayerMaxHP > 0)
		{
			mVDPlayerHP.GetData(TmpData.iPlayerHP);
			mVDActiveTime.GetData(TmpData.qwActiveTime);
			mVDCutsceneTime.GetData(TmpData.qwCutsceneTime);
			mVDPausedTime.GetData(TmpData.qwPausedTime);

			for (i = 0; i < MAX_ENTITY_COUNT; ++i)
			{
				if (mEntityMaxHPList[i].GetData(TmpData.EntityMaxHPList[i]) == false)
				{
					mEntityMaxHPList[i].InvalidateDataAddr();
					continue;
				}

				bData = mEntityHPList[i].GetData(TmpData.EntityHPList[i]);

				mEntityMaxHPList[i].InvalidateDataAddr();
				mEntityHPList[i].InvalidateDataAddr();

				if (bData == false)
					continue;

				++TmpData.iEntityCount;
			}
		}
		else
		{
			mLastDmg = 0;
			mVDActiveTime.InvalidateDataAddr();
			mVDCutsceneTime.InvalidateDataAddr();
			mVDPausedTime.InvalidateDataAddr();
			mVDPlayerHP.InvalidateDataAddr();
		}

		EnterCriticalSection(&mCSData);
		memcpy(&mREData, &TmpData, sizeof(RE_DATA));
		LeaveCriticalSection(&mCSData);

		mVDPlayerMaxHP.InvalidateDataAddr();
		mVDIsInControl.InvalidateDataAddr();

		Sleep(UPDATE_DATA_DELAY);
	} while (1);

	ExitThread(0);
}

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
//
// With VS 11, we could load up prebuilt .cso files instead...
//--------------------------------------------------------------------------------------
HRESULT REFramework::CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	std::cout << "Compiling from file..." << std::endl;

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
							dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			std::cout << "Error Compiling from file: " << pErrorBlob->GetBufferPointer() << std::endl;
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}

HRESULT REFramework::InitShaders()
{
	HRESULT hr = S_OK;

	auto device = mD3D11Hook->getDevice();

	// Compile the vertex shader
	std::cout << "Compiling vertex shader..." << std::endl;

	ID3DBlob* pVSBlob = nullptr;
	hr = CompileShaderFromFile(L"Tutorial02.fx", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		std::cout << "The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file." << std::endl;
		return hr;
	}

	// Create the vertex shader
	std::cout << "Creating vertex shader..." << std::endl;

	hr = device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &mPtrVertexShader);
	if (FAILED(hr))
	{
		std::cout << "Error on CreateVertexShader." << std::endl;
		pVSBlob->Release();
		return hr;
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	std::cout << "Creating input layout..." << std::endl;

	hr = device->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &mPtrVertexLayout);

	pVSBlob->Release();
	if (FAILED(hr))
	{
		std::cout << "Error on Release" << std::endl;
		return hr;
	}

	// Compile the pixel shader
	std::cout << "Compiling pixel shader..." << std::endl;

	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"Tutorial02.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		std::cout << "The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file." << std::endl;
		return hr;
	}

	// Create the pixel shader
	std::cout << "Creating pixel shader..." << std::endl;

	hr = device->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &mPtrPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
	{
		std::cout << "Error on Release." << std::endl;
		return hr;
	}

	// Create vertex buffer
	SimpleVertex vertices[] =
	{
		DirectX::XMFLOAT3(0.0f, 0.5f, 0.5f),
		DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f),
		DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f),
	};
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 3;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = vertices;
	hr = device->CreateBuffer(&bd, &InitData, &mPtrVertexBuffer);
	if (FAILED(hr))
	{
		std::cout << "Error on CreateBuffer." << std::endl;
		return hr;
	}

	return hr;
}

bool REFramework::Setup()
{
	if (mSetup)
		return true;

	std::cout << "Setting up REFramework..." << std::endl;

	//if (InitShaders() != S_OK)
	//{
	//	std::cout << "Failed to initialize shaders." << std::endl;
	//	return false;
	//}

	auto device = mD3D11Hook->getDevice();
	auto swapchain = mD3D11Hook->getSwapChain();

	if (device == nullptr || swapchain == nullptr)
	{
		MessageBox(0, "Device or SwapChain null. DirectX 12 may be in use.", "RE2Tool", MB_ICONERROR);
		return false;
	}

	ID3D11DeviceContext* pContext = nullptr;
	device->GetImmediateContext(&pContext);

	DXGI_SWAP_CHAIN_DESC swapDesc {};
	swapchain->GetDesc(&swapDesc);

	mWnd = swapDesc.OutputWindow;

	CreateRenderTarget();

	// Setup the viewport
	RECT rect = {};
	if (GetWindowRect(mWnd, &rect))
	{
		mViewport.Width = static_cast<float>(rect.right - rect.left);
		mViewport.Height = static_cast<float>(rect.bottom - rect.top);
	}
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;
	mViewport.TopLeftX = 0.0f;
	mViewport.TopLeftY = 0.0f;

	mHitmInf.X = (mViewport.Width / 2.0f) + (mViewport.Width * HITMARK_X_FACTOR);
	mHitmInf.Y = (mViewport.Height / 2.0f) - (mViewport.Height * HITMARK_Y_FACTOR);
	mHitmInf.fFontSize = HITMARK_FONT_FACTOR * mViewport.Height;
	mHitmInf.dwColor = HITMARK_COLOR;

	//((mViewport.Width / 2.0f) + HITMARK_X_OFFSET)
	//((mViewport.Height / 2.0f) - (HITMARK_Y_OFFSET + (i * HITMARK_FONT_SIZE)))

	if (mPtrFW1Factory == nullptr)
	{
		FW1CreateFactory(FW1_VERSION, &mPtrFW1Factory);
		mPtrFW1Factory->CreateFontWrapper(mD3D11Hook->getDevice(), nullptr, &mFontParams, &mPtrFontWrapper);
		mPtrFW1Factory->Release();
	}

	mWindowsMessageHook.reset();
	mWindowsMessageHook = std::make_unique<WindowsMessageHook>(mWnd);
	mWindowsMessageHook->OnMessage = [this](auto wnd, auto msg, auto wParam, auto lParam)
	{
		return OnMessage(wnd, msg, wParam, lParam);
	};

	if (!mInputHooked)
	{
		mDInputHook = std::make_unique<DInputHook>(mWnd);
		mInputHooked = true;
	}
	else
		mDInputHook->setWindow(mWnd);

	// Create ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	if (!ImGui_ImplWin32_Init(mWnd))
	{
		MessageBox(0, "Failed to initialize ImGui.", "RE2Tool", MB_ICONERROR);
		return false;
	}

	if (!ImGui_ImplDX11_Init(device, pContext))
	{
		MessageBox(0, "Failed to initialize ImGui.", "RE2Tool", MB_ICONERROR);
		return false;
	}

	ImGui::StyleColorsDark();

	mSetup = true;
	mStatWnd = true;
	mHitmark = true;
	mResetTimer = false;

	auto& io = ImGui::GetIO();

	io.WantCaptureKeyboard = false;
	io.WantCaptureMouse = false;
	io.MouseDrawCursor = false;

	io.ConfigFlags = ImGuiConfigFlags_NoMouse;

	mDInputHook->acknowledgeInput();

	if (this->mUpdateDataThreadHnd == nullptr)
		this->mUpdateDataThreadHnd = CreateThread(nullptr, 0, this->StartUpdateDataThread, this, 0, nullptr);

	return true;
}

void REFramework::CleanupRenderTarget()
{
	if (mPtrRenderTargetView != nullptr)
	{
		mPtrRenderTargetView->Release();
		mPtrRenderTargetView = nullptr;
	}
}

void REFramework::CreateRenderTarget()
{
	ID3D11DeviceContext* pContext = nullptr;
	mD3D11Hook->getDevice()->GetImmediateContext(&pContext);

	CleanupRenderTarget();

	ID3D11Texture2D* backBuffer {nullptr};
	if (mD3D11Hook->getSwapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer) == S_OK)
	{
		mD3D11Hook->getDevice()->CreateRenderTargetView(backBuffer, NULL, &mPtrRenderTargetView);
		backBuffer->Release();
	}
}

void REFramework::OnRender()
{
	if (mSetup == false)
	{
		if (!Setup())
		{
			std::cout << "Failed to initialize REFramework." << std::endl;
			return;
		}
	}

	static ID3D11DeviceContext* pContext = nullptr;
	//static std::vector<INT> DmgList;

	mD3D11Hook->getDevice()->GetImmediateContext(&pContext);

	//pContext->ClearRenderTargetView(g_mainRenderTargetView, DirectX::Colors::MidnightBlue);
	pContext->OMSetRenderTargets(1, &mPtrRenderTargetView, NULL);
	pContext->RSSetViewports(1, &mViewport);

	// Draw triangle
	//context->IASetInputLayout(g_pVertexLayout);
	//UINT stride = sizeof(SimpleVertex);
	//UINT offset = 0;
	//context->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	//context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//context->VSSetShader(g_pVertexShader, nullptr, 0);
	//context->PSSetShader(mPtrPixelShader, nullptr, 0);
	//context->Draw(3, 0);

	// Draw ImGui
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	EnterCriticalSection(&mCSInput);
	DrawUI();
	if (mDmgList.empty() == false && mHitmark == true)
		DrawHitmark(pContext);
	LeaveCriticalSection(&mCSInput);

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void REFramework::OnReset()
{
	std::cout << "Buffer resized." << std::endl;
	if (mPtrRenderTargetView != nullptr)
	{
		mPtrRenderTargetView->Release();
		mPtrRenderTargetView = nullptr;
	}
	mSetup = false;
}

void REFramework::DrawUI()
{
	auto& io = ImGui::GetIO();
	auto& style = ImGui::GetStyle();

	if (mStatWnd && mStatWndCorner != -1)
	{
		ImVec2 WndPos = ImVec2((mStatWndCorner & 1) ? io.DisplaySize.x - STATWND_DIST : STATWND_DIST, (mStatWndCorner & 2) ? io.DisplaySize.y - STATWND_DIST : STATWND_DIST);
		ImVec2 WndPosPivot = ImVec2((mStatWndCorner & 1) ? 1.0f : 0.0f, (mStatWndCorner & 2) ? 1.0f : 0.0f);

		style.FrameRounding = 2.0f;
		ImGui::SetNextWindowSize(ImVec2(215.0f, 0.0f));
		ImGui::SetNextWindowBgAlpha(0.30f);
		ImGui::SetNextWindowPos(WndPos, ImGuiCond_Always, WndPosPivot);

		if (ImGui::Begin("RE2Tool", &mStatWnd, (mStatWndCorner != -1 ? ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
		{
			static RE_DATA TmpData;
			static char buf[MAX_BUF_SIZE];

			if (ImGui::CollapsingHeader("Game performance", ImGuiTreeNodeFlags_DefaultOpen))
			{
				static float fps = 0.0f;
				static float max_fps = START_MAX_FPS;
				static int max_fps_count = 0;
				static float fps_list[90] = {0};
				static int list_index = 0;
				static double time_start = 0.0;
				static double time_now = 0.0;
				static double time_diff = 0.0;

				fps = io.Framerate;

				if (time_start == 0.0)
					time_start = ImGui::GetTime();
				else
				{
					time_now = ImGui::GetTime();
					time_diff = time_now - time_start;
					if (time_diff >= GRAPH_REFRESH_INTERVAL)
					{
						time_start = 0.0;
						fps_list[list_index] = fps;
						list_index = (list_index + 1) % IM_ARRAYSIZE(fps_list);

						if (fps > max_fps)
						{
							++max_fps_count;
							if (max_fps_count >= MAX_FPS_COUNT)
							{
								max_fps = fps;
								max_fps_count = 0;
							}
						}
					}
				}
				sprintf_s(buf, "fps: %.2f", fps);
				ImGui::PlotLines("", fps_list, IM_ARRAYSIZE(fps_list), list_index, buf, 0.0f, max_fps, ImVec2(198.0f, 60.0f), 4, true);
			}

			EnterCriticalSection(&mCSData);
			memcpy(&TmpData, &mREData, sizeof(RE_DATA));
			LeaveCriticalSection(&mCSData);

			if (TmpData.bIsInControl == 1 && TmpData.iPlayerMaxHP > 0)
			{
				static int i;
				static bool bHeaderOpen = false;
				static float fProgress = 0.0f;

				if (ImGui::CollapsingHeader("General info", ImGuiTreeNodeFlags_DefaultOpen))
				{
					TmpData.GetFormatedTime(buf);
					ImGui::Text("Time   :");
					ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), buf);

					ImGui::Text("Hitmark:");
					ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
					if (mHitmark)
						ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "ON");
					else
						ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "OFF");

					ImGui::Text("Damage :");
					ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
					if (mLastDmg >= 0)
						ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%i", mLastDmg);
					else
						ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%i", mLastDmg);

					ImGui::Text("Health :");
					ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
					sprintf_s(buf, "%d/%d", TmpData.iPlayerHP, TmpData.iPlayerMaxHP);
					fProgress = static_cast<float>(TmpData.iPlayerHP) / static_cast<float>(TmpData.iPlayerMaxHP);
					ImGui::ProgressBar(fProgress, ImVec2(-1.0f, 15.0f), buf);
				}

				if (TmpData.iEntityCount > 0)
				{
					sprintf_s(buf, "Entities health (%i)", TmpData.iEntityCount);
					bHeaderOpen = ImGui::CollapsingHeader(buf, ImGuiTreeNodeFlags_DefaultOpen);
				}
				if (bHeaderOpen)
				{
					for (i = 0; i < TmpData.iEntityCount; ++i)
					{
						sprintf_s(buf, "%d/%d", TmpData.EntityHPList[i], TmpData.EntityMaxHPList[i]);
						fProgress = static_cast<float>(TmpData.EntityHPList[i]) / static_cast<float>(TmpData.EntityMaxHPList[i]);
						ImGui::ProgressBar(fProgress, ImVec2(-1.0f, 15.0f), buf);
					}
				}
			}
			ImGui::End();
		}
	}
}

void REFramework::DrawHitmark(ID3D11DeviceContext* &pContext)
{
	static wchar_t buf[MAX_BUF_SIZE];
	static double time_start = 0.0;
	static double time_now = 0.0;
	static double time_diff = 0.0;

	if (time_start == 0.0 || mResetTimer)
	{
		time_start = ImGui::GetTime();
		mResetTimer = false;
	}
	else
	{
		if (mLastSize < mDmgList.size())
			time_start = ImGui::GetTime();

		time_now = ImGui::GetTime();
		time_diff = time_now - time_start;
		if (time_diff >= HITMARK_DISPLAY_INTERVAL)
		{
			mDmgList.clear();
			time_start = 0.0;
			return;
		}
	}

	static int i;
	for (i = static_cast<int>(mDmgList.size() - 1); i > -1; --i)
	{
		swprintf_s(buf, L"%i", mDmgList[i]);
		mPtrFontWrapper->DrawString(pContext, buf, mHitmInf.fFontSize, mHitmInf.X, mHitmInf.Y - (mHitmInf.fFontSize * i), mHitmInf.dwColor, FW1_RESTORESTATE);
	}

	mLastSize = mDmgList.size();
}

void WINAPI REFramework::OnDamageReceived(QWORD qwP1, QWORD qwP2, QWORD qwP3)
{
	//INT iDamage = *(reinterpret_cast<INT*>(qwP3 + 0x7C));
	auto OnDmgRecv = (decltype(REFramework::OnDamageReceived)*)gREFramework->mOnDmgRecvHook->getOriginal();
	INT iDamage = static_cast<INT>(qwP3);

	if (iDamage > 1 || iDamage < 0)
	{
		EnterCriticalSection(&gREFramework->mCSInput);
		
		gREFramework->mLastDmg = iDamage;
		if (gREFramework->mHitmark)
		{
			if (gREFramework->mDmgList.size() == DMG_LIST_CAPACITY)
				gREFramework->mDmgList.clear();
			gREFramework->mDmgList.push_back(iDamage);
		}

		LeaveCriticalSection(&gREFramework->mCSInput);
	}

	//gREFramework->mPtrDamageFunction(qwP1, qwP2, qwP3);
	OnDmgRecv(qwP1, qwP2, qwP3);
}
