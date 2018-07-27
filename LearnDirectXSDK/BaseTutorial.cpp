#include <d3d11.h>
#include <d3dx11.h>
#include <windows.h>
#include "Resource.h"
#include <tchar.h>
#include <xnamath.h>
#include <d3dcompiler.h>

struct SimpleVertex
{
	XMFLOAT3 pos;
};

HINSTANCE g_HInst = NULL;
HWND      g_Hwnd = NULL;
D3D_DRIVER_TYPE g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device *g_d3dDevice = NULL;
ID3D11DeviceContext *g_pImmediateContext = NULL;
IDXGISwapChain *g_pSwapChain = NULL;
ID3D11RenderTargetView *g_pRenderTargetView = NULL;
//Prepare for shader
ID3D11VertexShader *g_pVertexShader = NULL;
ID3D11PixelShader *g_pPixelShader = NULL;
ID3D11InputLayout *g_pVertexLayout = NULL;
ID3D11Buffer *g_pVertexBuffer = NULL;

HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);

HRESULT InitDevice();
void CleanDevice();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();

 int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
 	_In_opt_ HINSTANCE hPrevInstance,
 	_In_ LPTSTR    lpCmdLine,
 	_In_ int       nCmdShow)
//int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE pInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	if (FAILED(InitWindow(hInstance,nCmdShow)))
	{
		return 0;
	}
	if (FAILED(InitDevice()))
	{
		return 0;
	}
	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render();
		}
	}
	CleanDevice();
	return (int)msg.wParam;
}

HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"TutorialWindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);

	if (!RegisterClassEx(&wcex))
	{
		return E_FAIL;
	}
	g_HInst = hInstance;
	RECT rc = { 0, 0, 640, 480 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_Hwnd = CreateWindow(L"TutorialWindowClass", L"Direct3D 11 Tutorial 1: Direct3D 11 Basics", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
		NULL);
	if (!g_Hwnd)
	{
		return E_FAIL;
	}
	ShowWindow(g_Hwnd, nCmdShow);
	return S_OK;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case  WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case  WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob ** ppBlotOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

	ID3DBlob *pErrorBlob;

	hr = D3DX11CompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel, dwShaderFlags, 0, NULL, ppBlotOut, &pErrorBlob, NULL);
	if (FAILED(hr))
	{
		if (pErrorBlob != NULL)
		{
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			if (pErrorBlob)	pErrorBlob->Release();
			return hr;
		}
	}
	if (pErrorBlob)	pErrorBlob->Release();
	return S_OK;
}

HRESULT InitDevice()
{
	HRESULT hr = S_OK;
	RECT rc;
	GetClientRect(g_Hwnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlag = 0;
#ifdef _DEBUG
	createDeviceFlag |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_DRIVER_TYPE driveTypes[] = {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE
	};
	UINT driverNum = ARRAYSIZE(driveTypes);
	D3D_FEATURE_LEVEL featureLevel[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
	UINT featureNum = ARRAYSIZE(featureLevel);

	DXGI_SWAP_CHAIN_DESC  swapDesc;
	ZeroMemory(&swapDesc, sizeof(swapDesc));
	swapDesc.BufferCount = 1;
	swapDesc.BufferDesc.Width = width;
	swapDesc.BufferDesc.Height = height;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.OutputWindow = g_Hwnd;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.SampleDesc.Quality = 0;
	swapDesc.Windowed = TRUE;

	for (UINT driverTypeIndex = 0; driverTypeIndex < driverNum; driverTypeIndex++)
	{
		g_driverType = driveTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(NULL, g_driverType, NULL, createDeviceFlag, featureLevel, featureNum, D3D11_SDK_VERSION,
			&swapDesc, &g_pSwapChain, &g_d3dDevice, &g_featureLevel, &g_pImmediateContext);
		if (SUCCEEDED(hr))
		{
			break;;
		}
	}
	if (FAILED(hr))
	{
		return hr;
	}

	//Create Render target view;
	ID3D11Texture2D *pBackBuffer = NULL;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *)&pBackBuffer);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = g_d3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
	{
		return hr;
	}
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);
	//set up the viewport

	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MaxDepth = 1.0f;
	vp.MinDepth = 0.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	ID3DBlob *pShaderOut = NULL;
	hr = CompileShaderFromFile(L"LearnDirectXShader_0.fx", "VS", "vs_4_0", &pShaderOut);
	if (FAILED(hr))
	{
		MessageBox(g_Hwnd, L"The Fx file cannot be compiled. please run this executable", L"Error", MB_OK);
		return hr;
	}
	hr = g_d3dDevice->CreateVertexShader(pShaderOut->GetBufferPointer(), pShaderOut->GetBufferSize(), NULL, &g_pVertexShader);
	if (FAILED(hr))
	{
		pShaderOut->Release();
		return hr;
	}


	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);
	hr = g_d3dDevice->CreateInputLayout(layout, numElements, pShaderOut->GetBufferPointer(), pShaderOut->GetBufferSize(), &g_pVertexLayout);
	pShaderOut->Release();
	if (FAILED(hr))
	{
		return hr;
	}

	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	ID3DBlob *pixelShaderOut = NULL;
	hr = CompileShaderFromFile(L"LearnDirectXShader_0.fx", "PS", "ps_4_0", &pixelShaderOut);
	if (FAILED(hr))
	{
		MessageBox(NULL, L"The FX File cannot be compiled. Please run this executable from the directory", L"Eror", MB_OK);
		return hr;
	}

	hr = g_d3dDevice->CreatePixelShader(pixelShaderOut->GetBufferPointer(), pixelShaderOut->GetBufferSize(), NULL, &g_pPixelShader);
	pixelShaderOut->Release();
	if (FAILED(hr))
	{
		return hr;
	}

	//Create vertex buffer;
	SimpleVertex vertices[] = {
		XMFLOAT3(0.0f,0.5f,0.5f),
		XMFLOAT3(0.5f,-0.5f,0.5f),
		XMFLOAT3(-0.5f,-0.5f,0.5f)
	};
	UINT vertexNum = ARRAYSIZE(vertices);
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * vertexNum;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(initData));
	initData.pSysMem = vertices;
	hr = g_d3dDevice->CreateBuffer(&bd, &initData, &g_pVertexBuffer);
	if (FAILED(hr))
	{
		return hr;
	}
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

 	return S_OK;
}

void Render()
{
	float clearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);
	//Render a triangle
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->Draw(3, 0);
	g_pSwapChain->Present(0, 0);
}

void CleanDevice()
{
	if (g_pImmediateContext)
		g_pImmediateContext->ClearState();
	if (g_pVertexBuffer)	g_pVertexBuffer->Release();
	if (g_pPixelShader)	g_pPixelShader->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();
	if (g_pVertexShader)	g_pVertexShader->Release();
	if (g_pRenderTargetView)
	{
		g_pRenderTargetView->Release();
	}
	if (g_pSwapChain)
	{
		g_pSwapChain->Release();
	}
	if (g_pImmediateContext)
	{
		g_pImmediateContext->Release();
	}
	if (g_d3dDevice)
	{
		g_d3dDevice->Release();
	}
}


