#include "AutoRenderer.h"

#include "DirectDrawRenderer.h"
#include "GDIRenderer.h"

#include <cstdio>

bool AutoRenderer::Init(IWindow* hostWindow)
{
    std::unique_ptr<IRenderer> directDraw(new DirectDrawRenderer());
    if (directDraw->Init(hostWindow))
    {
        printf("Using DirectDraw renderer.\n");
        Active = std::move(directDraw);
        return true;
    }
    directDraw->Destroy();

    printf("DirectDraw unavailable, falling back to GDI renderer.\n");
    std::unique_ptr<IRenderer> gdi(new GDIRenderer());
    if (!gdi->Init(hostWindow))
    {
        return false;
    }

    Active = std::move(gdi);
    return true;
}

void AutoRenderer::Destroy()
{
    if (Active)
    {
        Active->Destroy();
        Active.reset();
    }
}

void AutoRenderer::BeginFrame()
{
    if (Active)
    {
        Active->BeginFrame();
    }
}

void AutoRenderer::DrawFrame(const unsigned char* rgbaPixels, int width, int height)
{
    if (Active)
    {
        Active->DrawFrame(rgbaPixels, width, height);
    }
}

void AutoRenderer::EndFrame()
{
    if (Active)
    {
        Active->EndFrame();
    }
}

void AutoRenderer::DisplaySetVSync(bool enabled)
{
    if (Active)
    {
        Active->DisplaySetVSync(enabled);
    }
}
