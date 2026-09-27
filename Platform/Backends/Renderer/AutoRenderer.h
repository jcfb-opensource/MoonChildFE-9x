#pragma once

#include "IRenderer.h"

#include <memory>

// Tries DirectDraw (hardware-accelerated blits) first, and falls back to the
// plain GDI renderer if DirectDraw isn't available/usable on this machine.
class AutoRenderer final : public IRenderer
{
public:
    bool Init(IWindow* hostWindow) override;
    void Destroy() override;

    void BeginFrame() override;
    void DrawFrame(const unsigned char* rgbaPixels, int width, int height) override;
    void EndFrame() override;

    void DisplaySetVSync(bool enabled) override;

private:
    std::unique_ptr<IRenderer> Active;
};
