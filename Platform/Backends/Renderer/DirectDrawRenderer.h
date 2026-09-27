#pragma once

#include "IRenderer.h"

#include <windows.h>
#include <ddraw.h>

class Win32Window;

// Hardware-accelerated renderer using DirectDraw (the API the original 1997
// game used) - lets the video card's blitter do the upscale + colour-space
// conversion instead of doing it in software on the CPU like GDIRenderer does.
// Falls back to GDI automatically (see AutoRenderer) if DirectDraw, or a
// capable driver, isn't available.
class DirectDrawRenderer final : public IRenderer
{
public:
    bool Init(IWindow* hostWindow) override;
    void Destroy() override;

    void BeginFrame() override;
    void DrawFrame(const unsigned char* rgbaPixels, int width, int height) override;
    void EndFrame() override;

    void DisplaySetVSync(bool enabled) override;

private:
    bool CreateOffscreenSurface(int width, int height);
    void ReleaseOffscreenSurface();

    Win32Window* Window = nullptr;

    LPDIRECTDRAW7 DD = nullptr;
    LPDIRECTDRAWSURFACE7 Primary = nullptr;
    LPDIRECTDRAWSURFACE7 Offscreen = nullptr;
    LPDIRECTDRAWCLIPPER Clipper = nullptr;

    int SurfaceWidth = 0;
    int SurfaceHeight = 0;
    bool VSyncEnabled = false;

    // Cached letterbox geometry so the black borders are only repainted when
    // the window is actually resized/moved, not every single frame.
    int LastWindowWidth = -1;
    int LastWindowHeight = -1;
    int LastScreenX = 0;
    int LastScreenY = 0;
    int LastDestX = 0;
    int LastDestY = 0;
    int LastDestWidth = 0;
    int LastDestHeight = 0;
};
