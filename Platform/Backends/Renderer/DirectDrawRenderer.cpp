// INITGUID must be defined before the first include that pulls in guiddef.h
// (via DirectDrawRenderer.h -> windows.h/ddraw.h) so IID_IDirectDraw7 etc. get
// instantiated here instead of requiring a separate dxguid/uuid import library.
#define INITGUID
#include "DirectDrawRenderer.h"

#include "IWindow.h"
#include "Win32Window.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

bool DirectDrawRenderer::Init(IWindow* hostWindow)
{
    Window = dynamic_cast<Win32Window*>(hostWindow);
    if (Window == nullptr)
    {
        return false;
    }

    LPDIRECTDRAW dd1 = nullptr;
    HRESULT hr = DirectDrawCreate(nullptr, &dd1, nullptr);
    if (FAILED(hr) || dd1 == nullptr)
    {
        printf("DirectDrawCreate failed! HRESULT 0x%08lx\n", static_cast<unsigned long>(hr));
        Window = nullptr;
        return false;
    }

    hr = dd1->QueryInterface(IID_IDirectDraw7, reinterpret_cast<void**>(&DD));
    dd1->Release();
    if (FAILED(hr) || DD == nullptr)
    {
        printf("IDirectDraw7 query failed! HRESULT 0x%08lx\n", static_cast<unsigned long>(hr));
        Window = nullptr;
        return false;
    }

    hr = DD->SetCooperativeLevel(Window->GetHWND(), DDSCL_NORMAL);
    if (FAILED(hr))
    {
        printf("SetCooperativeLevel failed! HRESULT 0x%08lx\n", static_cast<unsigned long>(hr));
        Destroy();
        return false;
    }

    DDSURFACEDESC2 primaryDesc = {};
    primaryDesc.dwSize = sizeof(primaryDesc);
    primaryDesc.dwFlags = DDSD_CAPS;
    primaryDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = DD->CreateSurface(&primaryDesc, &Primary, nullptr);
    if (FAILED(hr) || Primary == nullptr)
    {
        printf("CreateSurface(primary) failed! HRESULT 0x%08lx\n", static_cast<unsigned long>(hr));
        Destroy();
        return false;
    }

    hr = DD->CreateClipper(0, &Clipper, nullptr);
    if (FAILED(hr) || Clipper == nullptr || FAILED(Clipper->SetHWnd(0, Window->GetHWND())) ||
        FAILED(Primary->SetClipper(Clipper)))
    {
        printf("Clipper setup failed! HRESULT 0x%08lx\n", static_cast<unsigned long>(hr));
        Destroy();
        return false;
    }

    LastWindowWidth = -1;
    LastWindowHeight = -1;
    return true;
}

void DirectDrawRenderer::ReleaseOffscreenSurface()
{
    if (Offscreen != nullptr)
    {
        Offscreen->Release();
        Offscreen = nullptr;
    }
    SurfaceWidth = 0;
    SurfaceHeight = 0;
}

bool DirectDrawRenderer::CreateOffscreenSurface(int width, int height)
{
    ReleaseOffscreenSurface();

    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    desc.dwWidth = static_cast<DWORD>(width);
    desc.dwHeight = static_cast<DWORD>(height);

    // 32-bit XRGB - identical memory layout to our BGRA framebuffer (see
    // Cvideo::ConvertPalToDib), so no per-pixel conversion is needed. DirectDraw
    // itself handles converting to the desktop's actual pixel format at Blt time,
    // using the video card's blitter where the driver supports it.
    desc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    desc.ddpfPixelFormat.dwRGBBitCount = 32;
    desc.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
    desc.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
    desc.ddpfPixelFormat.dwBBitMask = 0x000000FF;

    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
    HRESULT hr = DD->CreateSurface(&desc, &Offscreen, nullptr);
    if (FAILED(hr))
    {
        // Not every HAL can do a 32-bit offscreen surface in video memory (e.g.
        // low-memory cards, or ones stuck in an 8/16-bit desktop mode) - system
        // memory always works, and sysmem->vidmem Blts are still HW accelerated
        // on most DirectDraw drivers.
        desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
        hr = DD->CreateSurface(&desc, &Offscreen, nullptr);
    }

    if (FAILED(hr) || Offscreen == nullptr)
    {
        printf("CreateSurface(offscreen) failed! HRESULT 0x%08lx\n", static_cast<unsigned long>(hr));
        return false;
    }

    SurfaceWidth = width;
    SurfaceHeight = height;
    return true;
}

void DirectDrawRenderer::Destroy()
{
    ReleaseOffscreenSurface();

    if (Clipper != nullptr)
    {
        Clipper->Release();
        Clipper = nullptr;
    }
    if (Primary != nullptr)
    {
        Primary->Release();
        Primary = nullptr;
    }
    if (DD != nullptr)
    {
        DD->Release();
        DD = nullptr;
    }
    Window = nullptr;
}

void DirectDrawRenderer::BeginFrame()
{
}

void DirectDrawRenderer::DrawFrame(const unsigned char* rgbaPixels, int width, int height)
{
    if (DD == nullptr || Primary == nullptr || Window == nullptr || width <= 0 || height <= 0)
    {
        return;
    }

    if (Offscreen == nullptr || width != SurfaceWidth || height != SurfaceHeight)
    {
        if (!CreateOffscreenSurface(width, height))
        {
            return;
        }
    }

    DDSURFACEDESC2 lockedDesc = {};
    lockedDesc.dwSize = sizeof(lockedDesc);
    HRESULT hr = Offscreen->Lock(nullptr, &lockedDesc, DDLOCK_WAIT | DDLOCK_NOSYSLOCK, nullptr);
    if (hr == DDERR_SURFACELOST)
    {
        Offscreen->Restore();
        hr = Offscreen->Lock(nullptr, &lockedDesc, DDLOCK_WAIT | DDLOCK_NOSYSLOCK, nullptr);
    }
    if (FAILED(hr))
    {
        return;
    }

    const size_t rowBytes = static_cast<size_t>(width) * 4;
    unsigned char* dst = static_cast<unsigned char*>(lockedDesc.lpSurface);
    if (static_cast<size_t>(lockedDesc.lPitch) == rowBytes)
    {
        std::memcpy(dst, rgbaPixels, rowBytes * static_cast<size_t>(height));
    }
    else
    {
        for (int y = 0; y < height; y++)
        {
            std::memcpy(dst + static_cast<size_t>(y) * lockedDesc.lPitch,
                rgbaPixels + static_cast<size_t>(y) * rowBytes, rowBytes);
        }
    }
    Offscreen->Unlock(nullptr);

    // The clipper needs screen coordinates, not window-client-relative ones.
    RECT clientRect;
    GetClientRect(Window->GetHWND(), &clientRect);
    POINT screenOrigin = {clientRect.left, clientRect.top};
    ClientToScreen(Window->GetHWND(), &screenOrigin);
    const int windowWidth = std::max<int>(1, clientRect.right - clientRect.left);
    const int windowHeight = std::max<int>(1, clientRect.bottom - clientRect.top);

    if (windowWidth != LastWindowWidth || windowHeight != LastWindowHeight ||
        screenOrigin.x != LastScreenX || screenOrigin.y != LastScreenY)
    {
        const float scaleX = static_cast<float>(windowWidth) / static_cast<float>(width);
        const float scaleY = static_cast<float>(windowHeight) / static_cast<float>(height);
        const float scale = std::min(scaleX, scaleY);

        LastDestWidth = static_cast<int>(width * scale);
        LastDestHeight = static_cast<int>(height * scale);
        LastDestX = screenOrigin.x + (windowWidth - LastDestWidth) / 2;
        LastDestY = screenOrigin.y + (windowHeight - LastDestHeight) / 2;

        DDBLTFX fillFx = {};
        fillFx.dwSize = sizeof(fillFx);
        fillFx.dwFillColor = 0;

        if (LastDestX > screenOrigin.x)
        {
            RECT left = {screenOrigin.x, screenOrigin.y, LastDestX, screenOrigin.y + windowHeight};
            RECT right = {LastDestX + LastDestWidth, screenOrigin.y, screenOrigin.x + windowWidth, screenOrigin.y + windowHeight};
            Primary->Blt(&left, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT, &fillFx);
            Primary->Blt(&right, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT, &fillFx);
        }
        if (LastDestY > screenOrigin.y)
        {
            RECT top = {screenOrigin.x, screenOrigin.y, screenOrigin.x + windowWidth, LastDestY};
            RECT bottom = {screenOrigin.x, LastDestY + LastDestHeight, screenOrigin.x + windowWidth, screenOrigin.y + windowHeight};
            Primary->Blt(&top, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT, &fillFx);
            Primary->Blt(&bottom, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT, &fillFx);
        }

        LastWindowWidth = windowWidth;
        LastWindowHeight = windowHeight;
        LastScreenX = screenOrigin.x;
        LastScreenY = screenOrigin.y;
    }

    if (VSyncEnabled)
    {
        DD->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, nullptr);
    }

    RECT destRect = {LastDestX, LastDestY, LastDestX + LastDestWidth, LastDestY + LastDestHeight};
    RECT srcRect = {0, 0, width, height};
    hr = Primary->Blt(&destRect, Offscreen, &srcRect, DDBLT_WAIT, nullptr);
    if (hr == DDERR_SURFACELOST)
    {
        Primary->Restore();
        Offscreen->Restore();
        Primary->Blt(&destRect, Offscreen, &srcRect, DDBLT_WAIT, nullptr);
    }
}

void DirectDrawRenderer::EndFrame()
{
}

void DirectDrawRenderer::DisplaySetVSync(bool enabled)
{
    VSyncEnabled = enabled;
}
