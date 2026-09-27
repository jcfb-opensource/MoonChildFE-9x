#ifndef _FRAMEWORK_KEYS_HPP
#define _FRAMEWORK_KEYS_HPP

// If <windows.h> was already included in this translation unit, it will have
// defined several of these as macros (with the same numeric values). Undef
// them first so the constexpr declarations below don't get mangled by the
// preprocessor.
#ifdef VK_BACK
#undef VK_BACK
#endif
#ifdef VK_TAB
#undef VK_TAB
#endif
#ifdef VK_RETURN
#undef VK_RETURN
#endif
#ifdef VK_SHIFT
#undef VK_SHIFT
#endif
#ifdef VK_ESCAPE
#undef VK_ESCAPE
#endif
#ifdef VK_LEFT
#undef VK_LEFT
#endif
#ifdef VK_UP
#undef VK_UP
#endif
#ifdef VK_RIGHT
#undef VK_RIGHT
#endif
#ifdef VK_DOWN
#undef VK_DOWN
#endif

static constexpr int VK_BACK   = 0x08;
static constexpr int VK_TAB    = 0x09;
static constexpr int VK_RETURN = 0x0D;
static constexpr int VK_SHIFT  = 0x10;
static constexpr int VK_CTRL   = 0x11;
static constexpr int VK_ALT    = 0x12;
static constexpr int VK_ESCAPE = 0x1B;
static constexpr int VK_LEFT   = 37;
static constexpr int VK_UP     = 38;
static constexpr int VK_RIGHT  = 39;
static constexpr int VK_DOWN   = 40;

static constexpr int CB_LEFT   = 240;
static constexpr int CB_RIGHT  = 241;
static constexpr int CB_UP     = 242;
static constexpr int CB_DOWN   = 243;
static constexpr int CB_JUMP   = 244;
static constexpr int CB_ACTION = 245;
static constexpr int CB_START  = 246;
static constexpr int CB_BACK   = 247;

#endif
