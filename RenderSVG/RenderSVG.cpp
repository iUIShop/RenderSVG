// RenderSVG.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "RenderSVG.h"
#include <windows.h>
#include <d2d1_3.h>
#include <d2d1svg.h>
#include <dwrite.h>
#include <shlwapi.h> // 用于 PathCombine 等路径操作
#include <d2d1_3.h>
#include <d2d1svg.h>

// 链接必要的库
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "shlwapi.lib")

// 全局变量
ID2D1HwndRenderTarget* g_pRenderTarget = nullptr;
// 新增：专门用于 SVG 操作的 DeviceContext5 接口
ID2D1DeviceContext5* g_pDeviceContext5 = nullptr;
ID2D1SvgDocument* g_pSvgDocument = nullptr;
// 窗口句柄
HWND g_hwnd = nullptr;



#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// 初始化 Direct2D 和加载 SVG
HRESULT InitD2D(HWND hwnd)
{
    HRESULT hr = S_OK;

    // --- 创建工厂 ---
    ID2D1Factory5* pD2DFactory = nullptr;
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory5), (void**)&pD2DFactory);
    if (FAILED(hr))
    {
        return hr;
    }

    // --- 创建渲染目标 ---
    D2D1_SIZE_U size = { 800, 600 };
    hr = pD2DFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd, size),
        &g_pRenderTarget
    );
    if (FAILED(hr))
    {
        pD2DFactory->Release();
        return hr;
    }

    // --- 获取 ID2D1DeviceContext5 ---
    // ID2D1HwndRenderTarget 继承自 ID2D1DeviceContext，我们可以查询出更高版本的接口
    hr = g_pRenderTarget->QueryInterface(__uuidof(ID2D1DeviceContext5), (void**)&g_pDeviceContext5);
    if (FAILED(hr))
    {
        MessageBox(hwnd, L"当前环境不支持 ID2D1DeviceContext5 (需要 Windows 10 或更新 SDK)", L"错误", MB_ICONERROR);
        return hr;
    }

    // --- 加载 SVG 文件 ---
    IStream* pStream = nullptr;
    // 确保 1.svg 在可执行文件同目录下
    hr = SHCreateStreamOnFileEx(LR"(PortraitMode-BvW15cvD.svg)", STGM_READ, 0, FALSE, nullptr, &pStream);

    if (SUCCEEDED(hr) && pStream)
    {
        // 使用 g_pDeviceContext5 调用 CreateSvgDocument
        // 注意：这里视口大小我设置为 500x500，你可以根据需要调整
        D2D1_SIZE_F viewBoxSize = D2D1::SizeF(500.0f, 500.0f);

        hr = g_pDeviceContext5->CreateSvgDocument(pStream, viewBoxSize, &g_pSvgDocument);

        pStream->Release();

        if (FAILED(hr))
        {
            MessageBox(hwnd, L"SVG 创建失败，请检查文件格式", L"错误", MB_ICONERROR);
        }
    }

    // 工厂不再需要，释放（RenderTarget 和 DeviceContext 会持有引用）
    pD2DFactory->Release();

    return hr;
}

// 渲染场景
void RenderScene()
{
    if (!g_pRenderTarget)
    {
        return;
    }

    g_pRenderTarget->BeginDraw();
    g_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

    if (g_pSvgDocument)
    {
        // 绘制 SVG
        g_pDeviceContext5->DrawSvgDocument(g_pSvgDocument);
    }

    HRESULT hr = g_pRenderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
    {
        // 处理资源丢失（简单示例中略）
    }
}

void Cleanup()
{
    if (g_pSvgDocument)
    {
        g_pSvgDocument->Release();
    }
    if (g_pDeviceContext5)
    {
        g_pDeviceContext5->Release();
    }
    if (g_pRenderTarget)
    {
        g_pRenderTarget->Release();
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_RENDERSVG, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RENDERSVG));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    Cleanup();

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RENDERSVG));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_RENDERSVG);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }
   g_hwnd = hWnd;

   // 初始化 Direct2D
   HRESULT hr = InitD2D(g_hwnd);
   if (FAILED(hr))
   {
       MessageBox(nullptr, L"Direct2D 初始化失败", L"错误", MB_ICONERROR);
       return -1;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        //{
        //    PAINTSTRUCT ps;
        //    HDC hdc = BeginPaint(hWnd, &ps);
        //    // TODO: Add any drawing code that uses hdc here...
        //    EndPaint(hWnd, &ps);
        //}
        RenderScene();
        break;

    case WM_SIZE:
        if (g_pRenderTarget)
        {
            // 窗口大小改变时调整渲染目标大小
            RECT rc;
            GetClientRect(hWnd, &rc);
            D2D1_SIZE_U newSize = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
            g_pRenderTarget->Resize(newSize);
            InvalidateRect(hWnd, nullptr, FALSE); // 强制重绘
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
