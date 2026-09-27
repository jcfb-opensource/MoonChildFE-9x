#include "PlatformBackends.h"

#include "Win32Window.h"
#include "AutoRenderer.h"
#include "Win32Input.h"
#include "WinMMAudio.h"

PlatformBackends MakeDefaultBackends()
{
    PlatformBackends backends;

    backends.Window.reset(new Win32Window());
    backends.Renderer.reset(new AutoRenderer());
    backends.Input.reset(new Win32Input());
    backends.Audio.reset(new WinMMAudio());

    return backends;
}
