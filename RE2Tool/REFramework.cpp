#include "REFramework.h"

// Commented out in original ImGui code
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

std::unique_ptr<REFramework> gREFramework {};

REFramework::REFramework()
	: mVDIsInControl(reinterpret_cast<BYTE*>(GetModuleHandle(0)) + 0x707A510, {0x408, 0xD8, 0x18, 0x20, 0x130}),
	  mVDMSTime     (reinterpret_cast<BYTE*>(GetModuleHandle(0)) + 0x70ACAE0, {0x2E0, 0x218, 0x610, 0x710, 0x60, 0x18}),
	  mVDPlayerMaxHP(reinterpret_cast<BYTE*>(GetModuleHandle(0)) + 0x70ACA88, {0x50, 0x20, 0x54}),
	  mVDPlayerHP   (reinterpret_cast<BYTE*>(GetModuleHandle(0)) + 0x70ACA88, {0x50, 0x20, 0x58})
{
	mInit = false;
	mStatWnd = false;
	mWnd = 0;
	mInputHooked = false;
	mStatWndCorner = 0;

	ZeroMemory(&mREData, sizeof(RE_DATA));

	mEntityMaxHPList.reserve(MAX_ENTITY_COUNT);
	mEntityHPList.reserve(MAX_ENTITY_COUNT);
	for (DWORD i = 0; i < MAX_ENTITY_COUNT; ++i)
	{
		mEntityMaxHPList.push_back(VirtualData<INT>(reinterpret_cast<BYTE*>(GetModuleHandle(0)) + 0x707B758, {0x80 + (i * 0x8), 0x88, 0x18, 0x1A0, 0x54}));
		mEntityHPList.push_back(VirtualData<INT>(reinterpret_cast<BYTE*>(GetModuleHandle(0)) + 0x707B758, {0x80 + (i * 0x8), 0x88, 0x18, 0x1A0, 0x58}));
	}
	
	// hook
	std::cout << "Hooking D3D11..." << std::endl;

	mD3D11Hook = std::make_unique<D3D11Hook>();
	mD3D11Hook->onPresent([this](D3D11Hook& hook)
	{
		OnRender();
	});
	mD3D11Hook->onResizeBuffers([this](D3D11Hook& hook)
	{
		OnReset();
	});

	if (mD3D11Hook->hook())
	{
		std::cout << "Hook completed!" << std::endl;
	}
	else
	{
		std::cout << "Hook failed." << std::endl;
	}
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

		if (TmpData.bIsInControl == 1)
		{
			mVDPlayerMaxHP.GetData(TmpData.iPlayerMaxHP);
			mVDPlayerHP.GetData(TmpData.iPlayerHP);
			mVDMSTime.GetData(TmpData.qwTime);

			for (i = 0; i < MAX_ENTITY_COUNT; ++i)
			{
				if (mEntityMaxHPList[i].GetData(TmpData.EntityMaxHPList[i]) == false)
				{
					mEntityMaxHPList[i].InvalidateDataAddr();
					break;
				}

				bData = mEntityHPList[i].GetData(TmpData.EntityHPList[i]);

				mEntityMaxHPList[i].InvalidateDataAddr();
				mEntityHPList[i].InvalidateDataAddr();

				if (bData == false)
					break;

				++TmpData.iEntityCount;
			}
		}

		mDataMutex.lock();
		memcpy(&mREData, &TmpData, sizeof(RE_DATA));
		mDataMutex.unlock();

		mVDPlayerMaxHP.InvalidateDataAddr();
		mVDPlayerHP.InvalidateDataAddr();
		mVDMSTime.InvalidateDataAddr();
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
		std::cout << "Error on Release" << std::endl;
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
		std::cout << "Error on CreateBuffer" << std::endl;
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
	auto swapChain = mD3D11Hook->getSwapChain();

	if (device == nullptr || swapChain == nullptr)
	{
		std::cout << "Device or SwapChain null. DirectX 12 may be in use." << std::endl;
		return false;
	}

	ID3D11DeviceContext* pContext = nullptr;
	device->GetImmediateContext(&pContext);

	DXGI_SWAP_CHAIN_DESC swapDesc {};
	swapChain->GetDesc(&swapDesc);

	mWnd = swapDesc.OutputWindow;

	CreateRenderTarget();

	// Setup the viewport
	mViewport.Width = 1920.0f;
	mViewport.Height = 1080.0f;
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;
	mViewport.TopLeftX = 0.0f;
	mViewport.TopLeftY = 0.0f;

	FW1CreateFactory(FW1_VERSION, &mPtrFW1Factory);
	mPtrFW1Factory->CreateFontWrapper(mD3D11Hook->getDevice(), L"Consolas", &mPtrFontWrapper);
	mPtrFW1Factory->Release();

	mWindowsMessageHook.reset();
	mWindowsMessageHook = std::make_unique<WindowsMessageHook>(mWnd);
	mWindowsMessageHook->onMessage = [this](auto wnd, auto msg, auto wParam, auto lParam)
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
	std::cout << "Initializing ImGui..." << std::endl;

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

	ID3D11DeviceContext* pContext = nullptr;
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

	// Color = ABGR
	//mPtrFontWrapper->DrawString(pContext, L"Press <INSERT> to open debug window", 16.0f, 0.0f, 0.0f, 0xFF1EFB52, FW1_RESTORESTATE);

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
	if (mPtrRenderTargetView != nullptr)
	{
		mPtrRenderTargetView->Release();
		mPtrRenderTargetView = nullptr;
	}
	mInit = false;
}

typedef char* POINTER;

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
			static char buf[32];

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

				//fps_list[list_index] = fps;
				//list_index = (list_index + 1) % IM_ARRAYSIZE(fps_list);

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

			if (mREData.bIsInControl == 1)
			{
				static int i;
				static RE_DATA TmpData;
				static bool bHeaderOpen = false;
				static float fProgress = 0.0f;

				mDataMutex.lock();
				memcpy(&TmpData, &mREData, sizeof(RE_DATA));
				mDataMutex.unlock();

				if (ImGui::CollapsingHeader("Player status", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Text("Time  : %i", TmpData.qwTime);
					ImGui::Text("Health:");
					ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);

					sprintf_s(buf, "%d/%d", TmpData.iPlayerHP, TmpData.iPlayerMaxHP);
					fProgress = static_cast<float>(TmpData.iPlayerHP) / static_cast<float>(TmpData.iPlayerMaxHP);
					ImGui::ProgressBar(fProgress, ImVec2(-1.0f, 15.0f), buf);
				}

				for (i = 0; i < TmpData.iEntityCount; ++i)
				{
					if (i == 0)
					{
						bHeaderOpen = ImGui::CollapsingHeader("Entities health", ImGuiTreeNodeFlags_DefaultOpen);
					}

					if (bHeaderOpen)
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
