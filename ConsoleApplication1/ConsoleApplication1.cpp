#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <gdiplus.h>
#include <ShellScalingAPI.h>

#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")
#pragma comment (lib, "Dwmapi.lib")
#pragma comment (lib, "Shcore.lib")

const wchar_t g_szClassName[] = L"myWindowClass";

#define RECTWIDTH(rc)			(rc.right - rc.left)
#define RECTHEIGHT(rc)			(rc.bottom - rc.top)

constexpr int TOPEXTENDWIDTH = 8;
constexpr int BOTTOMEXTENDWIDTH = 8;
constexpr int RIGHTEXTENDWIDTH = 8;
constexpr int LEFTEXTENDWIDTH = 8;

// TODO[F]: Adjust according to the known system-wide border width.
const int SystemBorderWidth = 1;

LRESULT HitTestNCA(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    // Get the point coordinates for the hit test.
    POINT ptMouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

    // Get the window rectangle.
    RECT rcWindow;
    GetWindowRect(hWnd, &rcWindow);

    // Get the frame rectangle, adjusted for the style without a caption.
    RECT rcFrame = { 0 };
    AdjustWindowRectEx(&rcFrame, WS_OVERLAPPEDWINDOW & ~WS_CAPTION, FALSE, NULL);

    // Determine if the hit test is for resizing. Default middle (1,1).
    USHORT uRow = 1;
    USHORT uCol = 1;
    bool fOnResizeBorder = false;

    // Determine if the point is at the top or bottom of the window.
    if (ptMouse.y >= rcWindow.top && ptMouse.y < rcWindow.top + TOPEXTENDWIDTH)
    {
        fOnResizeBorder = (ptMouse.y < (rcWindow.top - rcFrame.top));
        uRow = 0;
    }
    else if (ptMouse.y < rcWindow.bottom && ptMouse.y >= rcWindow.bottom - BOTTOMEXTENDWIDTH)
    {
        uRow = 2;
    }

    // Determine if the point is at the left or right of the window.
    if (ptMouse.x >= rcWindow.left && ptMouse.x < rcWindow.left + LEFTEXTENDWIDTH)
    {
        uCol = 0; // left side
    }
    else if (ptMouse.x < rcWindow.right && ptMouse.x >= rcWindow.right - RIGHTEXTENDWIDTH)
    {
        uCol = 2; // right side
    }

    // Hit test (HTTOPLEFT, ... HTBOTTOMRIGHT)
    LRESULT hitTests[3][3] =
    {
        { HTTOPLEFT,    fOnResizeBorder ? HTTOP : HTCAPTION,    HTTOPRIGHT },
        { HTLEFT,       HTNOWHERE,     HTRIGHT },
        { HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT },
    };

    return hitTests[uRow][uCol];
}

// Step 4: the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        // Handle window creation.
    case WM_CREATE:
    {
        RECT rcClient;
        GetWindowRect(hwnd, &rcClient);

        // Inform application of the frame change.
        SetWindowPos(hwnd,
                     NULL,
                     rcClient.left, rcClient.top,
                     RECTWIDTH(rcClient), RECTHEIGHT(rcClient),
                     SWP_FRAMECHANGED);

        BOOL isUseImmersiveDarkMode{ TRUE };
        HRESULT result = DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &isUseImmersiveDarkMode, sizeof(isUseImmersiveDarkMode));
        if (FAILED(result))
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &isUseImmersiveDarkMode, sizeof(isUseImmersiveDarkMode));

        break;
    }
        case WM_PAINT:
        {
                    PAINTSTRUCT ps;

            auto dc = BeginPaint(hwnd, &ps);

            for (int y = 0; y < SystemBorderWidth; ++y)
            {
                // TODO[F]: Window width should be used instead of 1500.
                for (int i = 0; i < 1500; ++i) SetPixel(dc, i, y, RGB(0, 0, 0));
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_NCHITTEST:
        {
            int result = HitTestNCA(hwnd, wParam, lParam);
            if (result == HTNOWHERE)
                break;
            else
                return result;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_NCCALCSIZE:
            if (wParam == TRUE)
            {
                NCCALCSIZE_PARAMS* pncsp = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);

                pncsp->rgrc[0].left = pncsp->rgrc[0].left + 8;
                pncsp->rgrc[0].top = pncsp->rgrc[0].top + 0;
                pncsp->rgrc[0].right = pncsp->rgrc[0].right - 8;
                pncsp->rgrc[0].bottom = pncsp->rgrc[0].bottom - 8;

                return 0;
            }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    WNDCLASSW wc;
    HWND hwnd;
    MSG Msg;

    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.lpszClassName = L"Window";
    wc.hInstance     = hInstance;
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH)); // BLACK_BRUSH
    wc.lpszMenuName  = NULL;
    wc.lpfnWndProc   = WndProc;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassW(&wc);

    hwnd = CreateWindowW(wc.lpszClassName, L"Window",
                         WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_POPUP,
                         100, 100, 350, 250, NULL, NULL, hInstance, NULL);

    if(hwnd == NULL)
    {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    MARGINS margins = { 0, 0, SystemBorderWidth, 0 } ;
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    return Msg.wParam;
}