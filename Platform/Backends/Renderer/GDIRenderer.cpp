#include "GDIRenderer.h"

#include "IWindow.h"
#include "Win32Window.h"

#include <algorithm>
#include <cstring>
#include <cstdio>

bool GDIRenderer::Init(IWindow* hostWindow)
{
    Window = dynamic_cast<Win32Window*>(hostWindow);
    if (Window == nullptr)
    {
        printf("Window is not a Win32Window!\n");
        return false;
    }

    WindowDC = GetDC(Window->GetHWND());
    if (WindowDC == nullptr)
    {
        printf("GetDC failed!\n");
        Window = nullptr;
        return false;
    }

    SourceWidth = 0;
    SourceHeight = 0;
    LastWindowWidth = -1;
    LastWindowHeight = -1;

    return true;
}

void GDIRenderer::Destroy()
{
    DestroyBackBuffer();

    if (Window != nullptr && WindowDC != nullptr)
    {
        ReleaseDC(Window->GetHWND(), WindowDC);
    }
    WindowDC = nullptr;
    Window = nullptr;
}

void GDIRenderer::DestroyBackBuffer()
{
    if (SectionBitmap != nullptr)
    {
        DeleteObject(SectionBitmap);
        SectionBitmap = nullptr;
    }
    if (MemoryDC != nullptr)
    {
        DeleteDC(MemoryDC);
        MemoryDC = nullptr;
    }
    SectionBits = nullptr;
}

void GDIRenderer::CreateBackBuffer(int width, int height)
{
    DestroyBackBuffer();

    MemoryDC = CreateCompatibleDC(WindowDC);

    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height; // top-down, matches our framebuffer layout

    SectionBitmap = CreateDIBSection(WindowDC, &info, DIB_RGB_COLORS, &SectionBits, nullptr, 0);
    SelectObject(MemoryDC, SectionBitmap);

    SourceWidth = width;
    SourceHeight = height;
}

void GDIRenderer::BeginFrame()
{
}

void GDIRenderer::DrawFrame(const unsigned char* rgbaPixels, int width, int height)
{
    if (WindowDC == nullptr || Window == nullptr || width <= 0 || height <= 0)
    {
        return;
    }

    if (MemoryDC == nullptr || SectionBits == nullptr || width != SourceWidth || height != SourceHeight)
    {
        CreateBackBuffer(width, height);
        if (SectionBits == nullptr)
        {
            printf("CreateDIBSection failed!\n");
            return;
        }
    }

    // The framebuffer is already BGRA (see Cvideo::ConvertPalToDib), matching what
    // 32bpp DIBs expect, so this is a straight memory copy - no per-pixel work.
    std::memcpy(SectionBits, rgbaPixels, static_cast<size_t>(width) * height * 4);

    const WindowSize windowSize = Window->GetPixelSize();
    const int windowWidth = std::max(1, windowSize.Width);
    const int windowHeight = std::max(1, windowSize.Height);

    if (windowWidth != LastWindowWidth || windowHeight != LastWindowHeight)
    {
        const float scaleX = static_cast<float>(windowWidth) / static_cast<float>(width);
        const float scaleY = static_cast<float>(windowHeight) / static_cast<float>(height);
        const float scale = std::min(scaleX, scaleY);

        LastDestWidth = static_cast<int>(width * scale);
        LastDestHeight = static_cast<int>(height * scale);
        LastDestX = (windowWidth - LastDestWidth) / 2;
        LastDestY = (windowHeight - LastDestHeight) / 2;

        if (LastDestX > 0)
        {
            PatBlt(WindowDC, 0, 0, LastDestX, windowHeight, BLACKNESS);
            PatBlt(WindowDC, LastDestX + LastDestWidth, 0, windowWidth - LastDestX - LastDestWidth, windowHeight, BLACKNESS);
        }
        if (LastDestY > 0)
        {
            PatBlt(WindowDC, 0, 0, windowWidth, LastDestY, BLACKNESS);
            PatBlt(WindowDC, 0, LastDestY + LastDestHeight, windowWidth, windowHeight - LastDestY - LastDestHeight, BLACKNESS);
        }

        LastWindowWidth = windowWidth;
        LastWindowHeight = windowHeight;
    }

    if (LastDestWidth == width && LastDestHeight == height)
    {
        // No scaling needed - a straight BitBlt is cheaper than StretchBlt.
        BitBlt(WindowDC, LastDestX, LastDestY, width, height, MemoryDC, 0, 0, SRCCOPY);
    }
    else
    {
        // COLORONCOLOR (nearest-neighbour) instead of HALFTONE - HALFTONE does a
        // software box-filter resample that's dramatically more expensive and was
        // the single biggest CPU cost on Pentium-class machines.
        SetStretchBltMode(WindowDC, COLORONCOLOR);
        StretchBlt(WindowDC,
            LastDestX, LastDestY, LastDestWidth, LastDestHeight,
            MemoryDC, 0, 0, width, height,
            SRCCOPY);
    }
}

void GDIRenderer::EndFrame()
{
}

void GDIRenderer::DisplaySetVSync(bool /*enabled*/)
{
    // GDI blits are not vsync-able on Windows 9x without DirectDraw; no-op.
}

