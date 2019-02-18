#include "REFramework.h"

std::unique_ptr<REFramework> gREFramework {};

REFramework::REFramework()
{
	mInit = false;
	mStatWnd = false;
	mWnd = 0;
	mInputHooked = false;
	mStatWndCorner = 0;
	mPtrFW1Factory = nullptr;
	pExecMem = nullptr;

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
	mFontParams.DefaultFontParams.pszFontFamily = L"Consolas";
	mFontParams.DefaultFontParams.FontWeight = DWRITE_FONT_WEIGHT_BOLD;
	mFontParams.DefaultFontParams.FontStyle = DWRITE_FONT_STYLE_NORMAL;
	mFontParams.DefaultFontParams.FontStretch = DWRITE_FONT_STRETCH_NORMAL;
	mFontParams.DefaultFontParams.pszLocale = L"";

	ZeroMemory(&mREData, sizeof(RE_DATA));

	// Initialize virtual memory pointers
	HMODULE BaseModuleAddr = GetModuleHandle(0);
	mVDIsInControl.SetDataPtr(reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x70B0E90, {0x408, 0xD8, 0x18, 0x20, 0x130});
	mVDActiveTime.SetDataPtr(reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x70AFEE8, {0x2E0, 0x218, 0x610, 0x710, 0x60, 0x18});
	mVDCutsceneTime.SetDataPtr(reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x70AFEE8, {0x2E0, 0x218, 0x610, 0x710, 0x60, 0x20});
	mVDPausedTime.SetDataPtr(reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x70AFEE8, {0x2E0, 0x218, 0x610, 0x710, 0x60, 0x30});
	mVDPlayerMaxHP.SetDataPtr(reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x70AFE10, {0x50, 0x20, 0x54});
	mVDPlayerHP.SetDataPtr(reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x70AFE10, {0x50, 0x20, 0x58});

	mEntityMaxHPList.reserve(MAX_ENTITY_COUNT);
	mEntityHPList.reserve(MAX_ENTITY_COUNT);
	for (DWORD i = 0; i < MAX_ENTITY_COUNT; ++i)
	{
		mEntityMaxHPList.push_back(VirtualData<INT>(reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x7081E50, {0x178 + (i * 0x8), 0x18, 0xB8, 0x54}));
		mEntityHPList.push_back   (VirtualData<INT>(reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x7081E50, {0x178 + (i * 0x8), 0x18, 0xB8, 0x58}));
		//mEntityMaxHPList.push_back(VirtualData<INT>(reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x7081EA8, {0x80 + (i * 0x8), 0x88, 0x18, 0x1A0, 0x54}));
		//mEntityHPList.push_back(VirtualData<INT>(reinterpret_cast<BYTE*>(BaseModuleAddr) + 0x7081EA8, {0x80 + (i * 0x8), 0x88, 0x18, 0x1A0, 0x58}));
	}
	
	// hook dmg function
	//std::cout << "Hooking RE2 damage function..." << std::endl;
	
	//BYTE* ModuleBaseAddr = nullptr;
	//BYTE* AOBAddr = nullptr;
	//DWORD dwModuleSize = 0;

	//OnDamageReceived(33);

	//if (memutil::GetModuleInfo(PROCESS_NAME, ModuleBaseAddr, dwModuleSize))
	//{
	//	AOBAddr = memutil::AOBScan(ModuleBaseAddr, dwModuleSize, PatternList[SigID::dmg_handle].aobPattern, PatternList[SigID::dmg_handle].dwPatternSize);
	//	pExecMem = memutil::AllocExecMem(AOB_DETOUR, sizeof(AOB_DETOUR));

	//	intptr_t iJumpParam = reinterpret_cast<intptr_t>(pExecMem);
	//	memcpy(AOB_JUMP + 2, &iJumpParam, sizeof(intptr_t));
	//	memutil::PatchExecMemory(AOBAddr, AOB_JUMP, sizeof(AOB_JUMP));
	//}

	//VirtualFree(pExecMem, 0, MEM_RELEASE);

	//if (MH_Initialize() != MH_OK)
	//{
	//	std::cout << "MH_Initialize failed." << std::endl;
	//}
	//// Create a hook for MessageBoxW, in disabled state.
	//if (MH_CreateHook(&MessageBoxA, &DetourFunction, reinterpret_cast<LPVOID*>(&ptrMessageBoxA)) != MH_OK)
	//{
	//	std::cout << "MH_CreateHook failed." << std::endl;
	//}
	//// Enable the hook for MessageBoxW.
	//if (MH_EnableHook(&MessageBoxA) != MH_OK)
	//{
	//	std::cout << "MH_EnableHook failed." << std::endl;
	//}

	//if (memutil::GetModuleInfo(PROCESS_NAME, ModuleBaseAddr, dwModuleSize))
	//{
	//	//DmgHandleAddr = memutil::AOBScan(ModuleBaseAddr, dwModuleSize, PatternList[SigID::dmg_handle].aobPattern, PatternList[SigID::dmg_handle].dwPatternSize);
	//	//DmgHandleAddr = reinterpret_cast<BYTE*>(0x14CA49A70);
	//	DmgHandleAddr = reinterpret_cast<BYTE*>(&MessageBoxA);
	//	if (DmgHandleAddr != nullptr)
	//	{
	//		mRE2DmgHandle = std::make_unique<FunctionHook>(&MessageBoxA, &DetourFunction);
	//		ptrMessageBoxA = mRE2DmgHandle->getOriginal<MESSAGEBOXA>();
	//	}
	//	else
	//		std::cout << "AOBScan failed." << std::endl;
	//}
	//else
	//	std::cout << "Failed to get module info." << std::endl;

	mD3D11Hook = std::make_unique<D3D11Hook>();
	mD3D11Hook->OnPresent([this](D3D11Hook& hook)
	{
		OnRender();
	});
	mD3D11Hook->OnResizeBuffers([this](D3D11Hook& hook)
	{
		OnReset();
	});

	mD3D11Hook->hook();
}

REFramework::~REFramework()
{
	if (this->mUpdateDataThreadHnd)
		TerminateThread(this->mUpdateDataThreadHnd, 0);
}

bool REFramework::OnMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (!mInit)
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
		std::lock_guard<std::mutex> guard(mInputMutex);

		++mStatWndCorner;
		if (mStatWndCorner > 3)
		{
			mStatWndCorner = -1;
			mStatWnd = false;
		}
		else
			mStatWnd = true;
	}

	mLastKeys = Keys;
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
			mVDActiveTime.InvalidateDataAddr();
			mVDCutsceneTime.InvalidateDataAddr();
			mVDPausedTime.InvalidateDataAddr();
			mVDPlayerHP.InvalidateDataAddr();
		}

		mDataMutex.lock();
		memcpy(&mREData, &TmpData, sizeof(RE_DATA));
		mDataMutex.unlock();

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

bool REFramework::Init()
{
	if (mInit)
		return true;

	std::cout << "Initializing REFramework..." << std::endl;

	//if (InitShaders() != S_OK)
	//{
	//	std::cout << "Failed to initialize shaders." << std::endl;
	//	return false;
	//}

	auto device = mD3D11Hook->getDevice();
	auto swapchain = mD3D11Hook->getSwapChain();

	if (device == nullptr || swapchain == nullptr)
	{
		std::cout << "Device or SwapChain null. DirectX 12 may be in use." << std::endl;
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
		std::cout << "Failed to initialize ImGui." << std::endl;
		return false;
	}

	if (!ImGui_ImplDX11_Init(device, pContext))
	{
		std::cout << "Failed to initialize ImGui." << std::endl;
		return false;
	}

	ImGui::StyleColorsDark();

	mInit = true;
	mStatWnd = true;

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
	if (mInit == false)
	{
		if (!Init())
		{
			std::cout << "Failed to initialize REFramework." << std::endl;
			return;
		}
	}

	static ID3D11DeviceContext* pContext = nullptr;
	mD3D11Hook->getDevice()->GetImmediateContext(&pContext);

	//pContext->ClearRenderTargetView(g_mainRenderTargetView, DirectX::Colors::MidnightBlue);

	pContext->OMSetRenderTargets(1, &mPtrRenderTargetView, NULL);

	//pContext->RSSetViewports(1, &mViewport);
	//DrawHitmark(pContext);

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

	DrawUI();

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
	mInit = false;
}

void REFramework::DrawUI()
{
	std::lock_guard<std::mutex> guard(mInputMutex);

	auto& io = ImGui::GetIO();
	auto& style = ImGui::GetStyle();

	io.WantCaptureKeyboard = false;
	io.WantCaptureMouse = false;
	io.MouseDrawCursor = false;
	io.ConfigFlags = ImGuiConfigFlags_NoMouse;

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

			mDataMutex.lock();
			memcpy(&TmpData, &mREData, sizeof(RE_DATA));
			mDataMutex.unlock();

			if (TmpData.bIsInControl == 1 && TmpData.iPlayerMaxHP > 0)
			{
				static int i;
				static bool bHeaderOpen = false;
				static float fProgress = 0.0f;

				if (ImGui::CollapsingHeader("General info", ImGuiTreeNodeFlags_DefaultOpen))
				{
					TmpData.GetFormatedTime(buf);
					ImGui::Text("Time  : %s", buf);
					ImGui::Text("Damage: %i", TmpData.iLastDmg);
					ImGui::Text("Health:");
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

	if (io.WantCaptureKeyboard)
	{
		mDInputHook->ignoreInput();
	}
	else
	{
		mDInputHook->acknowledgeInput();
	}
}

void REFramework::DrawHitmark(ID3D11DeviceContext* &pContext)
{
	// Color = ABGR
	mPtrFontWrapper->DrawString(pContext, L"200", 20.0f, ((mViewport.Width / 2.0f) - 0), ((mViewport.Height / 2.0f) - 50.0f), 0xFF1EFB52, FW1_RESTORESTATE);
}

void WINAPI OnDamageReceived(int iDamage)
{
	std::cout << "Damage received: " << iDamage << std::endl;
}
