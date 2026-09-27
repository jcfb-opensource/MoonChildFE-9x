# Moon Child FE (Win98 Edition)

Moon Child FE (Friend Edition) is a source port of the 1997 Windows 95 classic, Moon Child.

This branch is a stripped-down fork targeting **Windows 98** specifically: it drops SDL2/SDL3,
OpenGL/GLES, and every non-Windows target (Linux, macOS, Web, Android) in favor of a small,
self-contained Win32 backend built on APIs that have existed since Windows 95/98:

- Windowing & input: plain Win32 (`user32`) + `winmm` joystick polling
- Rendering: GDI `StretchDIBits` blitting (no DirectDraw/Direct3D/OpenGL required)
- Audio: `winmm` `waveOut` mixing

Built with the `i686-w64-mingw32` GCC toolchain (MSYS2's `mingw32` environment).

This port is based on the game's later iOS release, with various features (like the menu layout and FMVs) restored from the original Windows 95 version.

Remember, you've got the power to be his friend!


## Build Guide

### Windows (mingw32 / Win98 target)

1. Install the tools you need:
    - [MSYS2](https://www.msys2.org/), with the `mingw32` package group (32-bit `i686-w64-mingw32` GCC):
      ```bash
      pacman -S --needed base-devel mingw-w64-i686-toolchain
      ```
      By default this installs to `C:\msys64\mingw32`. If yours lives elsewhere, set the
      `MOONCHILD_MINGW32_ROOT` environment variable to that path before building.
    - [CMake](https://cmake.org/) (added to `PATH`)
    - Git
2. Open the Command Prompt, PowerShell, or Windows Terminal.
3. Run the `cd` command, followed by the path of the folder where you want to keep the source code, in quotation marks. For example: `cd "C:\GitHub"`.
4. Clone the repository with submodules and enter it:

```bat
git clone --recursive https://github.com/MorsGames/MoonChildFE.git
cd MoonChildFE
```

5. You have two options here:
    1. Run `Scripts\BuildGameWindows.bat` (add `Release` for a release build).
    2. Use the CMake presets directly: `cmake --preset win98-debug` then `cmake --build --preset build-win98-debug`.

The executable will end up in the `Bin\Win98` folder, alongside a `data` folder copied from `Data\`.

Note: only the Windows/mingw32 target is supported in this fork - Linux, macOS, Web, and Android
support (along with SDL2/SDL3/OpenGL) have been removed. See git history for the original
cross-platform CMakeLists.txt if you need it back.

## Credits

**Source Port by**: [Mors](https://mors.games) & Moon Child FE Contributors

**Original Game Code**: [Reinier van Vliet](https://www.proofofconcept.nl)

**Original Game Graphics**: [Metin Seven](https://www.metinseven.nl)

**Original Game Music & Sounds**: [Ramon Braumuller](https://open.spotify.com/artist/6ljLO5A329ym1FARh4xAz4?si=I2-mmFi4Qq-CLNZvoku7Pw)

**Additional Contributions**: Eidolon, pyrox0, koluckirafal, SteelT1, WickedSmoke, novadragonDOTspace, azphina

**Special Thanks**: Eidolon, AlbertHamik
