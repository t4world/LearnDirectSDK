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
	XMFLOAT4 color;
};

struct ConstantBuffer
{
	XMMATRIX world;
	XMMATRIX view;
	XMMATRIX projection;

};

HINSTANCE g_HInst = NULL;
HWND      g_Hwnd = NULL;
D3D_DRIVER_TYPE g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device *g_d3dDevice = NULL;
ID3D11DeviceContext *g_pImmediateContext = NULL;
IDXGISwapChain *g_pSwapChain = NULL;
ID3D11RenderTargetView *g_pRenderTargetView = NULL;
ID3D11Texture2D *g_pDepthStencil = NULL;
ID3D11DepthStencilView *g_pDepthStencilView = NULL;
//Prepare for shader
ID3D11VertexShader *g_pVertexShader = NULL;
ID3D11PixelShader *g_pPixelShader = NULL;
ID3D11InputLayout *g_pVertexLayout = NULL;
ID3D11Buffer *g_pVertexBuffer = NULL;
ID3D11Buffer *g_pIndexBuffer = NULL;
ID3D11Buffer *g_constantsBuffer = NULL;
XMMATRIX g_world;
XMMATRIX g_view;
XMMATRIX g_projection;
XMMATRIX g_world2;

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
	//Create depth stencil texture;
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_d3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencil);
	if (FAILED(hr))
	{
		return hr;
	}
	D3D11_DEPTH_STENCIL_VIEW_DESC descView;
	ZeroMemory(&descView, sizeof(descView));
	descView.Format = descDepth.Format;
	descView.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descView.Texture2D.MipSlice = 0;
	hr = g_d3dDevice->CreateDepthStencilView(g_pDepthStencil, &descView, &g_pDepthStencilView);
	if (FAILED(hr))
	{
		return hr;
	}
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
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
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
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
	//Create Index Buffer;

	WORD indices[] = {
		3, 1, 0,
		2, 1, 3,

		0, 5, 4,
		1, 5, 0,

		3, 4, 7,
		0, 4, 3,

		1, 6, 5,
		2, 6, 1,

		2, 7, 6,
		3, 7, 2,

		6, 4, 5,
		7, 4, 6,
	};
	UINT indexLength = ARRAYSIZE(indices);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD)* indexLength;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	initData.pSysMem = indices;
	hr = g_d3dDevice->CreateBuffer(&bd, &initData, &g_pIndexBuffer);
	if (FAILED(hr))
	{
		return hr;
	}

	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	//Create constant buffer;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_d3dDevice->CreateBuffer(&bd, NULL, &g_constantsBuffer);
	if (FAILED(hr))
	{
		return hr;
	}


	g_world = XMMatrixIdentity();
	g_world2 = XMMatrixIdentity();
	XMVECTOR eye = XMVectorSet(0.0f, 1.0f, -5.0f, 0.0f);
	XMVECTOR at = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	g_view = XMMatrixLookAtLH(eye, at, up);

	g_projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, width / (FLOAT)height, 0.1f, 100.0f);

 	return S_OK;
}

void Render()
{
	static float t = 0.0f;
	if (g_driverType == D3D_DRIVER_TYPE_REFERENCE)
	{
		t += (float)XM_PI * 0.0125f;
	}
	else
	{
		static DWORD dwTimeSet = 0;
		DWORD dwTimeCur = GetTickCount();
		if (dwTimeSet == 0)
		{
			dwTimeSet = dwTimeCur;
		}
		t = (dwTimeCur - dwTimeSet) / 1000.0f;
	}

	g_world = XMMatrixRotationY(t);

	XMMATRIX mSpin = XMMatrixRotationZ(-t);
	XMMATRIX mOrbit = XMMatrixRotationY(-t * 2.0f);
	XMMATRIX mtranlate = XMMatrixTranslation(-4.0f, 0.0f, 0.0f);
	XMMATRIX mScale = XMMatrixScaling(0.3f, 0.3f, 0.3f);
	g_world2 = mScale * mSpin * mtranlate * mOrbit;
	float clearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);
	//Clear the depth buffer to 1.0f
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	ConstantBuffer cb;
	cb.world = XMMatrixTranspose(g_world);
	cb.view = XMMatrixTranspose(g_view);
	cb.projection = XMMatrixTranspose(g_projection);
	g_pImmediateContext->UpdateSubresource(g_constantsBuffer, 0, NULL, &cb, 0, 0);
	//Render a triangle
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_constantsBuffer);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->DrawIndexed(36, 0, 0);

	ConstantBuffer cb2;
	cb2.world = XMMatrixTranspose(g_world2);
	cb2.view = XMMatrixTranspose(g_view);
	cb2.projection = XMMatrixTranspose(g_projection);
	g_pImmediateContext->UpdateSubresource(g_constantsBuffer, 0, NULL, &cb2, 0, 0);

	g_pImmediateContext->DrawIndexed(36, 0, 0);
	//g_pImmediateContext->Draw(3, 0);
	g_pSwapChain->Present(0, 0);
}

void CleanDevice()
{
	if (g_pImmediateContext)
		g_pImmediateContext->ClearState();
	if (g_constantsBuffer)	g_constantsBuffer->Release();
	if (g_pIndexBuffer)		g_pIndexBuffer->Release();
	if (g_pVertexBuffer)	g_pVertexBuffer->Release();
	if (g_pPixelShader)	g_pPixelShader->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();
	if (g_pVertexShader)	g_pVertexShader->Release();
	if (g_pRenderTargetView)
	{
		g_pRenderTargetView->Release();
	}
	if (g_pDepthStencil) g_pDepthStencil->Release();
	if (g_pDepthStencilView) g_pDepthStencilView->Release();
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


