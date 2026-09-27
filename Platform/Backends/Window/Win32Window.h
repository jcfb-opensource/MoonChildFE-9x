#pragma once

#include "IWindow.h"
#include "IInput.h"

#include <windows.h>

class Win32Window final : public IWindow
{
public:
    Win32Window();
    ~Win32Window() override;

    bool Create(const char* title, int width, int height) override;
    void Destroy() override;

    WindowSize GetPixelSize() const override;

    HWND GetHWND() const { return Hwnd; }

    void DisplaySetFullscreen(bool enabled) override;
    void SetCursorVisibility(bool visible) override;
    void SetRelativeMouseMode(bool enabled) override;

    void PumpOSEvents(IInput* sink, bool& outExitRequested) override;

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam);

    void CenterCursor();
    void PollJoystick();

    static Win32Window* Instance;

    HWND Hwnd = nullptr;
    IInput* Sink = nullptr;
    bool* ExitRequestedPtr = nullptr;

    bool RelativeMouseMode = false;
    bool CursorVisible = true;
    bool IsFullscreen = false;
    bool HasFocus = true;

    LONG_PTR WindowedStyle = 0;
    RECT WindowedRect = {};

    int ClientWidth = 640;
    int ClientHeight = 480;

    int LastMouseX = 0;
    int LastMouseY = 0;

    bool HasJoystick = false;
    UINT JoystickId = 0;
    DWORD PrevButtons = 0;
    int PrevAxisX = 0;
    int PrevAxisY = 0;
};
