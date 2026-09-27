# Moon Child FE

Moon Child FE (Friend Edition) is a modern source port of the 1997 Windows 95 classic, Moon Child. It's designed to be the definitive way to experience the game on modern hardware.

Differences from the original Windows 95 release include (but not limited to):

- Proper support for the latest versions of Windows, Linux, and macOS
- A full web version that you can play in your browser
- Automatic saving and loading of progress
- Controller support
- Independent input mapping for "Up" and "Jump"
- Fullscreen and windowed display modes
- 50FPS and 60FPS toggle, with Vsync support
- Nice and shimmer-free image scaling
- Brand new Speedrun Mode, which adds a speedrun timer to the top left corner of the screen
- Brand new Easier Shooting option, which makes the jetpack shooting controls more intuitive
- Brand new Safe Visuals option, which makes the final level slightly easier on the eyes
- Slightly higher quality audio
- Alternate title screen music :D
- Updated credits
- And more!

This port is based on the game's later iOS release, with various features (like the menu layout and FMVs) restored from the original Windows 95 version.

Remember, you've got the power to be his friend!


## Windows 98 Edition (this fork)

This fork is a stripped-down variant that targets **Windows 98** specifically, in the
spirit of the original 1997 release. Compared to the mainline project above, it:

- Drops SDL2/SDL3, OpenGL/GLES, and every non-Windows target (Linux, macOS, Web, Android)
- Replaces them with a small, self-contained Win32 backend built entirely on APIs that
  have existed since Windows 95/98:
  - Windowing & input: plain Win32 (`user32`) + `winmm` joystick polling
  - Rendering: **DirectDraw** hardware-accelerated blitting (matching what the original
    1997 game used), automatically falling back to GDI `StretchDIBits`/`BitBlt` software
    blitting if DirectDraw isn't available
  - Audio: `winmm` `waveOut` mixing
- Builds with the `i686-w64-mingw32` GCC toolchain (e.g. MSYS2's `mingw32` environment,
  or any similarly-prefixed cross toolchain), instead of MSVC/clang-cl

See the Windows build instructions below for how to build this fork; the Linux/macOS/Web
build instructions further down describe the mainline (non-Win98) project and no longer
apply here, since those platforms/toolchains were removed from this fork.


## Build Guide

### Windows (Win98 Edition / mingw32)

1. Install the tools you need:
    - [MSYS2](https://www.msys2.org/), with the `mingw32` package group (32-bit
      `i686-w64-mingw32` GCC):
      ```bash
      pacman -S --needed base-devel mingw-w64-i686-toolchain
      ```
      By default this installs to `C:\msys64\mingw32`. If yours lives elsewhere (e.g. a
      custom cross-built toolchain on Linux/macOS), set the `MOONCHILD_MINGW32_ROOT`
      environment variable to that path before building, or pass
      `-DMOONCHILD_MINGW32_ROOT=...` to CMake.
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
    2. Use the CMake presets directly: `cmake --preset win98-debug` then `cmake --build --preset build-win98-debug` (or `win98-release`/`build-win98-release`).

The executable will end up in the `Bin\Win98` folder, alongside a `data` folder copied from `Data\`.


### Linux (mainline project only - not part of this fork)

1. Open a terminal and install the required packages. On Debian or Ubuntu based distros, you can do so by running:

```bash
sudo apt update
sudo apt install -y --no-install-recommends git cmake build-essential ninja-build clang pkg-config libasound2-dev libpulse-dev libaudio-dev libjack-dev libsndio-dev libfribidi-dev libthai-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxxf86vm-dev libxtst-dev libxkbcommon-dev libwayland-dev wayland-protocols libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev
```

2. Run the `cd` command, followed by the path of the folder where you want to keep the source code. For example: `cd ~/GitHub`.
3. Clone the repository with submodules and enter it:

```bash
git clone --recursive https://github.com/MorsGames/MoonChildFE.git
cd MoonChildFE
```

4. You have two options here:
    1. Run a build script manually:
        - For a 64-bit build, run `bash ./Scripts/BuildGameUnix.sh LinuxX64`
        - For an ARM64 build, run `bash ./Scripts/BuildGameUnix.sh LinuxArm64` (UNTESTED)
        - You can also add `Release` to the end of the command to build the game in release mode.
    2. Use the CMake commands directly. If you're taking this route, you probably don't need a guide in the first place.

The executable will be end up in the `Bin` folder.

### macOS (mainline project only - not part of this fork)

1. Install the [Xcode Command Line Tools](https://developer.apple.com/documentation/xcode/installing-the-command-line-tools/) and [Homebrew](https://brew.sh/).
2. Install CMake and Ninja from Homebrew.

```bash
brew install cmake ninja
```

3. Run the `cd` command, followed by the path of the folder where you want to keep the source code. For example: `cd ~/GitHub`.
4. Clone the repository with submodules and enter it:

```bash
git clone --recursive https://github.com/MorsGames/MoonChildFE.git
cd MoonChildFE
```

4. You have two options here:
    1. Run a build script manually:
        - For a Universal Binary (x64 + arm64) build, run `bash ./Scripts/BuildGameUnix.sh macOSUniversal`
        - You can also add `Release` to the end of the command to build the game in release mode.
    2. Use the CMake commands directly. If you're taking this route, you probably don't need a guide in the first place.

The executable will be end up in the `Bin` folder.
 

## Credits

**Source Port by**: [Mors](https://mors.games) & Moon Child FE Contributors

**Original Game Code**: [Reinier van Vliet](https://www.proofofconcept.nl)

**Original Game Graphics**: [Metin Seven](https://www.metinseven.nl)

**Original Game Music & Sounds**: [Ramon Braumuller](https://open.spotify.com/artist/6ljLO5A329ym1FARh4xAz4?si=I2-mmFi4Qq-CLNZvoku7Pw)

**Additional Contributions**: Eidolon, pyrox0, koluckirafal, SteelT1, WickedSmoke, novadragonDOTspace, azphina

**Special Thanks**: Eidolon, AlbertHamik
