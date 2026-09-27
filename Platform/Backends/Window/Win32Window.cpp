#include "Win32Window.h"

#include "DisplayBridge.h"

#include <mmsystem.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

#pragma comment(lib, "winmm.lib")

constexpr float GAME_WIDTH = 640.0f;
constexpr float GAME_HEIGHT = 480.0f;

Win32Window* Win32Window::Instance = nullptr;

namespace
{
    struct GameViewport
    {
        float X = 0.0f;
        float Y = 0.0f;
        float Width = 0.0f;
        float Height = 0.0f;
    };

    GameViewport GetGameViewport(int windowWidth, int windowHeight)
    {
        GameViewport viewport;
        if (windowWidth <= 0 || windowHeight <= 0)
        {
            return viewport;
        }

        const float scaleX = static_cast<float>(windowWidth) / GAME_WIDTH;
        const float scaleY = static_cast<float>(windowHeight) / GAME_HEIGHT;
        const float scale = std::min(scaleX, scaleY);

        viewport.Width = GAME_WIDTH * scale;
        viewport.Height = GAME_HEIGHT * scale;
        viewport.X = (static_cast<float>(windowWidth) - viewport.Width) * 0.5f;
        viewport.Y = (static_cast<float>(windowHeight) - viewport.Height) * 0.5f;
        return viewport;
    }

    void ScaleAbsoluteCoordinates(int windowWidth, int windowHeight, float inX, float inY, float& outX, float& outY)
    {
        const GameViewport viewport = GetGameViewport(windowWidth, windowHeight);
        if (viewport.Width <= 0.0f || viewport.Height <= 0.0f)
        {
            outX = inX;
            outY = inY;
            return;
        }

        const float normalizedX = (inX - viewport.X) / viewport.Width;
        const float normalizedY = (inY - viewport.Y) / viewport.Height;
        const float clampedX = std::max(0.0f, std::min(1.0f, normalizedX));
        const float clampedY = std::max(0.0f, std::min(1.0f, normalizedY));

        outX = clampedX * GAME_WIDTH;
        outY = clampedY * GAME_HEIGHT;
    }

    void ScaleRelativeCoordinates(int windowWidth, int windowHeight, float inX, float inY, float& outX, float& outY)
    {
        const GameViewport viewport = GetGameViewport(windowWidth, windowHeight);
        if (viewport.Width <= 0.0f || viewport.Height <= 0.0f)
        {
            outX = inX;
            outY = inY;
            return;
        }

        outX = inX * (GAME_WIDTH / viewport.Width);
        outY = inY * (GAME_HEIGHT / viewport.Height);
    }
}

Win32Window::Win32Window() = default;

Win32Window::~Win32Window()
{
    Destroy();
}

LRESULT CALLBACK Win32Window::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (Instance != nullptr && Instance->Hwnd == hwnd)
    {
        return Instance->HandleMessage(msg, wparam, lparam);
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

LRESULT Win32Window::HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        {
            if (ExitRequestedPtr != nullptr)
            {
                *ExitRequestedPtr = true;
            }
            return 0;
        }

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            if (Sink != nullptr)
            {
                const bool isDown = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
                const bool isRepeat = isDown && ((lparam & 0x40000000) != 0);
                Sink->OnKeyEvent(static_cast<int>(wparam), isDown, isRepeat);
            }
            // Let DefWindowProc handle ALT+F4 etc, but avoid the default beep for other keys
            if (msg == WM_SYSKEYDOWN && wparam == VK_F4)
            {
                break;
            }
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            if (Sink != nullptr)
            {
                const int x = static_cast<short>(LOWORD(lparam));
                const int y = static_cast<short>(HIWORD(lparam));

                RECT client;
                GetClientRect(Hwnd, &client);
                const int windowWidth = client.right - client.left;
                const int windowHeight = client.bottom - client.top;

                if (RelativeMouseMode)
                {
                    const int centerX = windowWidth / 2;
                    const int centerY = windowHeight / 2;
                    const int rawDx = x - centerX;
                    const int rawDy = y - centerY;
                    if (rawDx != 0 || rawDy != 0)
                    {
                        float scaledDx = 0.0f;
                        float scaledDy = 0.0f;
                        ScaleRelativeCoordinates(windowWidth, windowHeight,
                            static_cast<float>(rawDx), static_cast<float>(rawDy), scaledDx, scaledDy);

                        float gameX = 0.0f;
                        float gameY = 0.0f;
                        ScaleAbsoluteCoordinates(windowWidth, windowHeight,
                            static_cast<float>(x), static_cast<float>(y), gameX, gameY);

                        Sink->OnMouseMovement(gameX, gameY, scaledDx, scaledDy);
                        CenterCursor();
                    }
                }
                else
                {
                    float gameX = 0.0f;
                    float gameY = 0.0f;
                    ScaleAbsoluteCoordinates(windowWidth, windowHeight,
                        static_cast<float>(x), static_cast<float>(y), gameX, gameY);

                    float scaledDx = 0.0f;
                    float scaledDy = 0.0f;
                    ScaleRelativeCoordinates(windowWidth, windowHeight,
                        static_cast<float>(x - LastMouseX), static_cast<float>(y - LastMouseY), scaledDx, scaledDy);

                    Sink->OnMouseMovement(gameX, gameY, scaledDx, scaledDy);
                    LastMouseX = x;
                    LastMouseY = y;
                }
            }
            return 0;
        }

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        {
            if (Sink != nullptr)
            {
                const int x = static_cast<short>(LOWORD(lparam));
                const int y = static_cast<short>(HIWORD(lparam));

                RECT client;
                GetClientRect(Hwnd, &client);
                float gameX = 0.0f;
                float gameY = 0.0f;
                ScaleAbsoluteCoordinates(client.right - client.left, client.bottom - client.top,
                    static_cast<float>(x), static_cast<float>(y), gameX, gameY);

                const bool isDown = (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN);
                const int button = (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP) ? INPUT_MOUSE_BUTTON_LEFT : INPUT_MOUSE_BUTTON_RIGHT;
                Sink->OnMouseButton(button, isDown, gameX, gameY);
            }
            return 0;
        }

        case WM_SETCURSOR:
        {
            if (LOWORD(lparam) == HTCLIENT)
            {
                SetCursor((!CursorVisible || RelativeMouseMode) ? nullptr : LoadCursorA(nullptr, IDC_ARROW));
                return TRUE;
            }
            break;
        }

        case WM_ACTIVATE:
        {
            const bool active = (LOWORD(wparam) != WA_INACTIVE);
            if (HasFocus && !active && Sink != nullptr)
            {
                Sink->OnFocusLost();
            }
            HasFocus = active;
            return 0;
        }

        case WM_ERASEBKGND:
        {
            return 1;
        }

        default:
        {
            break;
        }
    }

    return DefWindowProcA(Hwnd, msg, wparam, lparam);
}

bool Win32Window::Create(const char* title, int width, int height)
{
    Instance = this;

    const HINSTANCE hInstance = GetModuleHandleA(nullptr);

    WNDCLASSA wc = {};
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &Win32Window::WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconA(hInstance, MAKEINTRESOURCEA(101));
    wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = "MoonChildFEWindowClass";

    if (!RegisterClassA(&wc))
    {
        printf("RegisterClassA failed! Error %lu\n", GetLastError());
        return false;
    }

    const DWORD style = WS_OVERLAPPEDWINDOW;
    RECT rect = {0, 0, width, height};
    AdjustWindowRect(&rect, style, FALSE);

    Hwnd = CreateWindowExA(0, wc.lpszClassName, title, style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, hInstance, nullptr);

    if (Hwnd == nullptr)
    {
        printf("CreateWindowExA failed! Error %lu\n", GetLastError());
        return false;
    }

    ClientWidth = width;
    ClientHeight = height;

    ShowWindow(Hwnd, SW_SHOW);
    UpdateWindow(Hwnd);

    JOYCAPSA caps = {};
    HasJoystick = (joyGetDevCapsA(JOYSTICKID1, &caps, sizeof(caps)) == JOYERR_NOERROR);
    JoystickId = JOYSTICKID1;

    return true;
}

void Win32Window::Destroy()
{
    if (Hwnd != nullptr)
    {
        DestroyWindow(Hwnd);
        Hwnd = nullptr;
    }
    Instance = nullptr;
}

WindowSize Win32Window::GetPixelSize() const
{
    WindowSize size;
    if (Hwnd != nullptr)
    {
        RECT client;
        GetClientRect(Hwnd, &client);
        size.Width = client.right - client.left;
        size.Height = client.bottom - client.top;
    }
    return size;
}

void Win32Window::DisplaySetFullscreen(bool enabled)
{
    if (enabled == IsFullscreen)
    {
        return;
    }

    if (enabled)
    {
        WindowedStyle = GetWindowLongPtrA(Hwnd, GWL_STYLE);
        GetWindowRect(Hwnd, &WindowedRect);

        const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        SetWindowLongPtrA(Hwnd, GWL_STYLE, static_cast<LONG_PTR>(WS_POPUP | WS_VISIBLE));
        SetWindowPos(Hwnd, HWND_TOP, 0, 0, screenWidth, screenHeight, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
    else
    {
        SetWindowLongPtrA(Hwnd, GWL_STYLE, WindowedStyle);
        SetWindowPos(Hwnd, HWND_NOTOPMOST,
            WindowedRect.left, WindowedRect.top,
            WindowedRect.right - WindowedRect.left, WindowedRect.bottom - WindowedRect.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }

    IsFullscreen = enabled;
    DisplayBridge::NotifyFullscreenChange(enabled ? 1 : 0);
}

void Win32Window::SetCursorVisibility(bool visible)
{
    CursorVisible = visible;
    SetCursor((!CursorVisible || RelativeMouseMode) ? nullptr : LoadCursorA(nullptr, IDC_ARROW));
}

void Win32Window::CenterCursor()
{
    RECT client;
    GetClientRect(Hwnd, &client);
    POINT center = {(client.right - client.left) / 2, (client.bottom - client.top) / 2};
    ClientToScreen(Hwnd, &center);
    SetCursorPos(center.x, center.y);
}

void Win32Window::SetRelativeMouseMode(bool enabled)
{
    if (RelativeMouseMode == enabled)
    {
        return;
    }

    RelativeMouseMode = enabled;

    if (enabled)
    {
        SetCapture(Hwnd);
        CenterCursor();
        SetCursor(nullptr);
    }
    else
    {
        ReleaseCapture();
        SetCursor(CursorVisible ? LoadCursorA(nullptr, IDC_ARROW) : nullptr);
    }
}

void Win32Window::PollJoystick()
{
    if (!HasJoystick || Sink == nullptr)
    {
        return;
    }

    JOYINFOEX info = {};
    info.dwSize = sizeof(info);
    info.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY;

    if (joyGetPosEx(JoystickId, &info) != JOYERR_NOERROR)
    {
        if (PrevButtons != 0 || PrevAxisX != 0 || PrevAxisY != 0)
        {
            Sink->OnGamepadDisconnected(static_cast<int>(JoystickId));
            PrevButtons = 0;
            PrevAxisX = 0;
            PrevAxisY = 0;
        }
        return;
    }

    static constexpr DWORD BUTTON_MASKS[4] = {0x1, 0x2, 0x4, 0x8};
    for (int i = 0; i < 4; i++)
    {
        const bool wasDown = (PrevButtons & BUTTON_MASKS[i]) != 0;
        const bool isDown = (info.dwButtons & BUTTON_MASKS[i]) != 0;
        if (wasDown != isDown)
        {
            Sink->OnGamepadButton(static_cast<int>(JoystickId), i, isDown);
        }
    }
    PrevButtons = info.dwButtons;

    // Normalize [0, 65535] axis range to roughly [-32768, 32767], centered on 32767
    const int axisX = static_cast<int>(info.dwXpos) - 32767;
    const int axisY = static_cast<int>(info.dwYpos) - 32767;
    if (axisX != PrevAxisX)
    {
        Sink->OnGamepadAxis(static_cast<int>(JoystickId), 0, axisX);
        PrevAxisX = axisX;
    }
    if (axisY != PrevAxisY)
    {
        Sink->OnGamepadAxis(static_cast<int>(JoystickId), 1, axisY);
        PrevAxisY = axisY;
    }
}

void Win32Window::PumpOSEvents(IInput* sink, bool& outExitRequested)
{
    Sink = sink;
    ExitRequestedPtr = &outExitRequested;

    MSG msg;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    PollJoystick();

    Sink = nullptr;
    ExitRequestedPtr = nullptr;
}
