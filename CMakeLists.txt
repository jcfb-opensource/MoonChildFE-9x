cmake_minimum_required(VERSION 3.22.1)

project(MoonChildFE LANGUAGES C CXX RC VERSION 1.1.1)
set(MOONCHILD_OUTPUT_NAME "Moon Child FE")
set(EXECUTABLE_NAME MoonChildFE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# This build only targets Windows 98 (and compatible 9x/NT systems) via the
# i686-w64-mingw32 GCC toolchain that ships with MSYS2. All other platforms
# (Linux, macOS, Web, Android) have been removed - see git history if you need them.
if(NOT WIN32)
    message(FATAL_ERROR "MoonChildFE (Win98 Edition) only supports Windows targets.")
endif()

# Paths
set(MOONCHILD_LIBRARY_DIR ${CMAKE_CURRENT_LIST_DIR}/Libraries)
set(MOONCHILD_CLASSES_DIR ${CMAKE_CURRENT_LIST_DIR}/Classes)
set(MOONCHILD_FRAMEWORK_DIR ${MOONCHILD_CLASSES_DIR}/framewrk)
set(MOONCHILD_GAME_DIR ${MOONCHILD_CLASSES_DIR}/moonchild)
set(MOONCHILD_PLATFORM_DIR ${CMAKE_CURRENT_LIST_DIR}/Platform)

# Build options
option(MOONCHILD_VENDORED_ZLIB "Use vendored zlib library" ON)
option(MOONCHILD_VENDORED_PL_MPEG "Use vendored pl_mpeg library" ON)

# zlib setup (used by fastfile/unzip for reading the .art/.des archives)
if(MOONCHILD_VENDORED_ZLIB)
    set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ZLIB_BUILD_SHARED   OFF CACHE BOOL "" FORCE)
    set(ZLIB_BUILD_STATIC   ON  CACHE BOOL "" FORCE)
    add_subdirectory("${MOONCHILD_LIBRARY_DIR}/zlib" "${CMAKE_BINARY_DIR}/zlib" EXCLUDE_FROM_ALL)
    set(MOONCHILD_ZLIB_TARGET zlibstatic)
else()
    find_package(ZLIB REQUIRED)
    set(MOONCHILD_ZLIB_TARGET ZLIB::ZLIB)
endif()

# Game sources
set(MOONCHILD_GAME_SOURCES
    ${MOONCHILD_GAME_DIR}/anim.cpp
    ${MOONCHILD_GAME_DIR}/asset.cpp
    ${MOONCHILD_GAME_DIR}/asteroid.cpp
    ${MOONCHILD_GAME_DIR}/basic.cpp
    ${MOONCHILD_GAME_DIR}/bat.cpp
    ${MOONCHILD_GAME_DIR}/bbot.cpp
    ${MOONCHILD_GAME_DIR}/bee.cpp
    ${MOONCHILD_GAME_DIR}/bolt.cpp
    ${MOONCHILD_GAME_DIR}/bonus.cpp
    ${MOONCHILD_GAME_DIR}/boss.cpp
    ${MOONCHILD_GAME_DIR}/bouncey.cpp
    ${MOONCHILD_GAME_DIR}/bullet.cpp
    ${MOONCHILD_GAME_DIR}/bump.cpp
    ${MOONCHILD_GAME_DIR}/cannon.cpp
    ${MOONCHILD_GAME_DIR}/chain.cpp
    ${MOONCHILD_GAME_DIR}/claw.cpp
    ${MOONCHILD_GAME_DIR}/cloud.cpp
    ${MOONCHILD_GAME_DIR}/diamond.cpp
    ${MOONCHILD_GAME_DIR}/doldoler.cpp
    ${MOONCHILD_GAME_DIR}/editor.cpp
    ${MOONCHILD_GAME_DIR}/element.cpp
    ${MOONCHILD_GAME_DIR}/elevat.cpp
    ${MOONCHILD_GAME_DIR}/glim.cpp
    ${MOONCHILD_GAME_DIR}/globals.cpp
    ${MOONCHILD_GAME_DIR}/goodday.cpp
    ${MOONCHILD_GAME_DIR}/gravlift.cpp
    ${MOONCHILD_GAME_DIR}/gumbal.cpp
    ${MOONCHILD_GAME_DIR}/hoi.cpp
    ${MOONCHILD_GAME_DIR}/ironauto.cpp
    ${MOONCHILD_GAME_DIR}/ironring.cpp
    ${MOONCHILD_GAME_DIR}/levinits.cpp
    ${MOONCHILD_GAME_DIR}/lift.cpp
    ${MOONCHILD_GAME_DIR}/mc.cpp
    ${MOONCHILD_GAME_DIR}/metbal.cpp
    ${MOONCHILD_GAME_DIR}/metcan.cpp
    ${MOONCHILD_GAME_DIR}/mine.cpp
    ${MOONCHILD_GAME_DIR}/mixer.cpp
    ${MOONCHILD_GAME_DIR}/mouth.cpp
    ${MOONCHILD_GAME_DIR}/mushroom.cpp
    ${MOONCHILD_GAME_DIR}/objects.cpp
    ${MOONCHILD_GAME_DIR}/paal.cpp
    ${MOONCHILD_GAME_DIR}/plant.cpp
    ${MOONCHILD_GAME_DIR}/plof.cpp
    ${MOONCHILD_GAME_DIR}/prefs.cpp
    ${MOONCHILD_GAME_DIR}/ptoei.cpp
    ${MOONCHILD_GAME_DIR}/punt.cpp
    ${MOONCHILD_GAME_DIR}/robyn.cpp
    ${MOONCHILD_GAME_DIR}/rock.cpp
    ${MOONCHILD_GAME_DIR}/rocket.cpp
    ${MOONCHILD_GAME_DIR}/score.cpp
    ${MOONCHILD_GAME_DIR}/scroll.cpp
    ${MOONCHILD_GAME_DIR}/seamine.cpp
    ${MOONCHILD_GAME_DIR}/smlheart.cpp
    ${MOONCHILD_GAME_DIR}/snake.cpp
    ${MOONCHILD_GAME_DIR}/sneak.cpp
    ${MOONCHILD_GAME_DIR}/sokoban.cpp
    ${MOONCHILD_GAME_DIR}/sound.cpp
    ${MOONCHILD_GAME_DIR}/spies.cpp
    ${MOONCHILD_GAME_DIR}/spike.cpp
    ${MOONCHILD_GAME_DIR}/stukhout.cpp
    ${MOONCHILD_GAME_DIR}/switsj.cpp
    ${MOONCHILD_GAME_DIR}/trigger.cpp
    ${MOONCHILD_GAME_DIR}/vgdll.cpp
    ${MOONCHILD_GAME_DIR}/warp.cpp
    ${MOONCHILD_GAME_DIR}/weight.cpp
    ${MOONCHILD_GAME_DIR}/wheel.cpp
    ${MOONCHILD_FRAMEWORK_DIR}/blitbuf.cpp
    ${MOONCHILD_FRAMEWORK_DIR}/fastfile.cpp
    ${MOONCHILD_FRAMEWORK_DIR}/pcxff.cpp
    ${MOONCHILD_FRAMEWORK_DIR}/sprite.cpp
    ${MOONCHILD_FRAMEWORK_DIR}/timer.cpp
    ${MOONCHILD_FRAMEWORK_DIR}/unzip.c
    ${MOONCHILD_FRAMEWORK_DIR}/video.cpp
)

# Platform sources
set(MOONCHILD_PLATFORM_SOURCES
    ${MOONCHILD_PLATFORM_DIR}/App/Main.cpp
    ${MOONCHILD_PLATFORM_DIR}/App/Host.cpp
    ${MOONCHILD_PLATFORM_DIR}/App/PlatformBackends.cpp
    ${MOONCHILD_PLATFORM_DIR}/App/Resources.cpp
    ${MOONCHILD_PLATFORM_DIR}/App/FrameworkGlue.cpp
    ${MOONCHILD_PLATFORM_DIR}/App/MediaCatalog.cpp
    ${MOONCHILD_PLATFORM_DIR}/Core/Random.cpp
    ${MOONCHILD_PLATFORM_DIR}/Bridges/AudioBridge.cpp
    ${MOONCHILD_PLATFORM_DIR}/Bridges/DisplayBridge.cpp
    ${MOONCHILD_PLATFORM_DIR}/Bridges/InputBridge.cpp
    ${MOONCHILD_PLATFORM_DIR}/Bridges/MovieBridge.cpp
    ${MOONCHILD_PLATFORM_DIR}/Audio/AudioEngine.cpp
    ${MOONCHILD_PLATFORM_DIR}/Audio/MP3MusicTrack.cpp
)

# Native Win32/GDI/WinMM backend (no SDL, no OpenGL, no DirectX - just plain Win32 APIs
# that have existed since Windows 95/98)
set(MOONCHILD_BACKEND_SOURCES
    ${MOONCHILD_PLATFORM_DIR}/Backends/Window/Win32Window.cpp
    ${MOONCHILD_PLATFORM_DIR}/Backends/Renderer/GDIRenderer.cpp
    ${MOONCHILD_PLATFORM_DIR}/Backends/Renderer/DirectDrawRenderer.cpp
    ${MOONCHILD_PLATFORM_DIR}/Backends/Renderer/AutoRenderer.cpp
    ${MOONCHILD_PLATFORM_DIR}/Backends/Input/Win32Input.cpp
    ${MOONCHILD_PLATFORM_DIR}/Backends/Audio/WinMMAudio.cpp
)

set(MOONCHILD_BACKEND_INCLUDES
    ${MOONCHILD_PLATFORM_DIR}/Backends/Window
    ${MOONCHILD_PLATFORM_DIR}/Backends/Renderer
    ${MOONCHILD_PLATFORM_DIR}/Backends/Input
    ${MOONCHILD_PLATFORM_DIR}/Backends/Audio
    ${MOONCHILD_PLATFORM_DIR}/Audio
    ${MOONCHILD_LIBRARY_DIR}/dr_libs
)

# Final sources list
set(MOONCHILD_SOURCES
    ${MOONCHILD_GAME_SOURCES}
    ${MOONCHILD_PLATFORM_SOURCES}
    ${MOONCHILD_BACKEND_SOURCES}
)

# Target
add_executable(${EXECUTABLE_NAME} WIN32 ${MOONCHILD_SOURCES})
set_target_properties(${EXECUTABLE_NAME} PROPERTIES OUTPUT_NAME "${MOONCHILD_OUTPUT_NAME}")

target_include_directories(${EXECUTABLE_NAME} PRIVATE
    ${MOONCHILD_CLASSES_DIR}
    ${MOONCHILD_FRAMEWORK_DIR}
    ${MOONCHILD_GAME_DIR}
    ${MOONCHILD_PLATFORM_DIR}/Core
    ${MOONCHILD_PLATFORM_DIR}/App
    ${MOONCHILD_PLATFORM_DIR}/Bridges
    ${MOONCHILD_BACKEND_INCLUDES}
)

target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
    "MOONCHILD_APP_VERSION_STR=\"${CMAKE_PROJECT_VERSION}\""
    MOONCHILD_WINDOWS
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    # Target the Windows 98 (Win32/Win9x "4.10") API level - avoids accidentally
    # linking against APIs that only exist on NT5+/2000+.
    WINVER=0x0410
    _WIN32_WINDOWS=0x0410
    _WIN32_WINNT=0x0400
)

# pl_mpeg setup (software MPEG-1 decoder used for FMVs, no OS dependency)
if(MOONCHILD_VENDORED_PL_MPEG)
    target_include_directories(${EXECUTABLE_NAME} PRIVATE
      ${MOONCHILD_LIBRARY_DIR}/pl_mpeg
    )
else()
    target_include_directories(${EXECUTABLE_NAME} PRIVATE
      $ENV{MOONCHILD_PL_MPEG_PATH}
    )
endif()

target_link_libraries(${EXECUTABLE_NAME} PRIVATE
    ${MOONCHILD_ZLIB_TARGET}
    winmm
    gdi32
    ddraw
    user32
    kernel32
    comdlg32
)

# Prefer static linking of the mingw/gcc runtime so the .exe doesn't depend on
# libgcc/libstdc++ DLLs being present on the target Windows 98 machine.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(${EXECUTABLE_NAME} PRIVATE -static -static-libgcc -static-libstdc++)

    # dr_mp3's file-opening code compiles in a wchar_t (_wfopen) code path that we
    # never actually call (we always use the char* drmp3_init_file API) - some
    # minimal/old mingw import libraries don't export _wfopen at all. Per-function
    # sections + linker GC drop that unused code before it can cause a link error.
    target_compile_options(${EXECUTABLE_NAME} PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(${EXECUTABLE_NAME} PRIVATE -Wl,--gc-sections)
endif()

# Version info + icon resource
configure_file(
    "${CMAKE_SOURCE_DIR}/Windows/MoonChild.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/MoonChild.rc"
    @ONLY
)
target_sources(${EXECUTABLE_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/MoonChild.rc")

# Output directory (MinGW Makefiles is a single-config generator, so there's
# no separate per-config subfolder - just build Debug/Release into their own binary dirs)
set(MOONCHILD_BIN_DIR ${CMAKE_SOURCE_DIR}/Bin/Win98)
set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${MOONCHILD_BIN_DIR}
)

set(MOONCHILD_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Data)
add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${MOONCHILD_RESOURCES_DIR}"
        $<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/data
)
