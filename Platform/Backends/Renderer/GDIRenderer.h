#pragma once

#include "IRenderer.h"

#include <windows.h>

class Win32Window;

// Straightforward GDI blitter - no OpenGL/DirectDraw required, works on Windows 95/98.
class GDIRenderer final : public IRenderer
{
public:
    bool Init(IWindow* hostWindow) override;
    void Destroy() override;

    void BeginFrame() override;
    void DrawFrame(const unsigned char* rgbaPixels, int width, int height) override;
    void EndFrame() override;

    void DisplaySetVSync(bool enabled) override;

private:
    void CreateBackBuffer(int width, int height);
    void DestroyBackBuffer();

    Win32Window* Window = nullptr;
    HDC WindowDC = nullptr;

    // Persistent memory DC + DIB section: writing pixels straight into mapped
    // section memory and BitBlt/StretchBlt-ing from it is quite a bit cheaper
    // on old GDI implementations than calling StretchDIBits from a raw buffer
    // every single frame.
    HDC MemoryDC = nullptr;
    HBITMAP SectionBitmap = nullptr;
    void* SectionBits = nullptr;

    int SourceWidth = 0;
    int SourceHeight = 0;

    // Cached letterbox geometry so we only repaint the black borders when the
    // window is actually resized, instead of every single frame.
    int LastWindowWidth = -1;
    int LastWindowHeight = -1;
    int LastDestX = 0;
    int LastDestY = 0;
    int LastDestWidth = 0;
    int LastDestHeight = 0;
};
