#include "Win32Input.h"

static constexpr int AXIS_THRESHOLD = 16000;

// Windows virtual-key codes already match the VK_* values expected by keys.hpp,
// so most keys can be forwarded as-is (see keys.hpp for the small custom subset).
static constexpr int VK_LEFT_NATIVE = 0x25;
static constexpr int VK_UP_NATIVE = 0x26;
static constexpr int VK_RIGHT_NATIVE = 0x27;
static constexpr int VK_DOWN_NATIVE = 0x28;
static constexpr int VK_RETURN_NATIVE = 0x0D;
static constexpr int VK_ESCAPE_NATIVE = 0x1B;
static constexpr int VK_BACK_NATIVE = 0x08;
static constexpr int VK_TAB_NATIVE = 0x09;
static constexpr int VK_SHIFT_NATIVE = 0x10;
static constexpr int VK_CONTROL_NATIVE = 0x11;
static constexpr int VK_MENU_NATIVE = 0x12;
static constexpr int VK_SPACE_NATIVE = 0x20;

Win32Input::Win32Input() = default;

Win32Input::~Win32Input()
{
    Destroy();
}

bool Win32Input::Init()
{
    return true;
}

void Win32Input::Destroy()
{
    ClearAllSources();
    Queue.push_back(InputEvent::FocusLost());
    MouseDeltaRemainderX = 0.0f;
    MouseDeltaRemainderY = 0.0f;
}

int Win32Input::TranslateKey(int nativeKeyCode)
{
    if (nativeKeyCode >= 'A' && nativeKeyCode <= 'Z')
    {
        return nativeKeyCode;
    }
    if (nativeKeyCode >= '0' && nativeKeyCode <= '9')
    {
        return nativeKeyCode;
    }

    switch (nativeKeyCode)
    {
        case VK_LEFT_NATIVE:    return VK_LEFT_NATIVE;
        case VK_RIGHT_NATIVE:   return VK_RIGHT_NATIVE;
        case VK_UP_NATIVE:      return VK_UP_NATIVE;
        case VK_DOWN_NATIVE:    return VK_DOWN_NATIVE;
        case VK_SPACE_NATIVE:   return VK_SPACE_NATIVE;
        case VK_RETURN_NATIVE:  return VK_RETURN_NATIVE;
        case VK_ESCAPE_NATIVE:  return VK_ESCAPE_NATIVE;
        case VK_BACK_NATIVE:    return VK_BACK_NATIVE;
        case VK_TAB_NATIVE:     return VK_TAB_NATIVE;
        case VK_SHIFT_NATIVE:   return VK_SHIFT_NATIVE;
        case VK_CONTROL_NATIVE: return VK_CONTROL_NATIVE;
        case VK_MENU_NATIVE:    return VK_MENU_NATIVE;
        default:                return 0;
    }
}

int Win32Input::TranslateGamepadButton(int button)
{
    switch (button)
    {
        case 0: return CB_JUMP;
        case 1: return CB_ACTION;
        case 2: return CB_BACK;
        case 3: return CB_START;
        default: return INPUT_CODE_NONE;
    }
}

void Win32Input::TranslateGamepadAxis(int axis, int& outNegativeCode, int& outPositiveCode)
{
    outNegativeCode = INPUT_CODE_NONE;
    outPositiveCode = INPUT_CODE_NONE;

    switch (axis)
    {
        case 0: // X axis
        {
            outNegativeCode = CB_LEFT;
            outPositiveCode = CB_RIGHT;
            break;
        }
        case 1: // Y axis
        {
            outNegativeCode = CB_UP;
            outPositiveCode = CB_DOWN;
            break;
        }
        default:
        {
            break;
        }
    }
}

void Win32Input::SetSource(uint32_t sourceId, int code, bool isDown)
{
    if (isDown && code == INPUT_CODE_NONE)
    {
        return;
    }

    Queue.push_back({sourceId, code, isDown});
}

void Win32Input::ClearAllSources()
{
    Queue.push_back({0, 0, false});
}

void Win32Input::OnKeyEvent(int nativeKeyCode, bool isDown, bool isRepeat)
{
    if (isRepeat)
    {
        return;
    }
    const int code = TranslateKey(nativeKeyCode);
    if (code == 0)
    {
        return;
    }
    const uint32_t sourceId = INPUT_SOURCE_KEY | static_cast<uint32_t>(nativeKeyCode & INPUT_SOURCE_CODE_MASK);
    SetSource(sourceId, code, isDown);
}

void Win32Input::OnMouseMovement(float x, float y, float xrel, float yrel)
{
    const float deltaX = xrel + MouseDeltaRemainderX;
    const float deltaY = yrel + MouseDeltaRemainderY;
    const int wholeDeltaX = static_cast<int>(deltaX);
    const int wholeDeltaY = static_cast<int>(deltaY);

    MouseDeltaRemainderX = deltaX - static_cast<float>(wholeDeltaX);
    MouseDeltaRemainderY = deltaY - static_cast<float>(wholeDeltaY);

    Queue.push_back(InputEvent::MouseMove(static_cast<int>(x), static_cast<int>(y), wholeDeltaX, wholeDeltaY));
}

void Win32Input::OnMouseButton(int button, bool isDown, float x, float y)
{
    Queue.push_back(InputEvent::MouseButton(button, isDown, static_cast<int>(x), static_cast<int>(y)));
}

void Win32Input::OnGamepadConnected(int /*instanceId*/)
{
}

void Win32Input::OnGamepadDisconnected(int /*instanceId*/)
{
    ClearAllSources();
}

void Win32Input::OnGamepadButton(int instanceId, int button, bool isDown)
{
    const int code = TranslateGamepadButton(button);
    if (code == 0)
    {
        return;
    }
    const uint32_t sourceId = INPUT_SOURCE_GAMEPAD_BUTTON | static_cast<uint32_t>((instanceId * 8 + button) & INPUT_SOURCE_CODE_MASK);
    SetSource(sourceId, code, isDown);
}

void Win32Input::OnGamepadAxis(int instanceId, int axis, int value)
{
    int negativeCode = INPUT_CODE_NONE;
    int positiveCode = INPUT_CODE_NONE;
    TranslateGamepadAxis(axis, negativeCode, positiveCode);
    if (negativeCode == INPUT_CODE_NONE && positiveCode == INPUT_CODE_NONE)
    {
        return;
    }

    const uint32_t negSourceId = INPUT_SOURCE_GAMEPAD_AXIS_NEG | static_cast<uint32_t>((instanceId * 8 + axis) & INPUT_SOURCE_CODE_MASK);
    const uint32_t posSourceId = INPUT_SOURCE_GAMEPAD_AXIS_POS | static_cast<uint32_t>((instanceId * 8 + axis) & INPUT_SOURCE_CODE_MASK);

    SetSource(negSourceId, negativeCode, value <= -AXIS_THRESHOLD);
    SetSource(posSourceId, positiveCode, value >= AXIS_THRESHOLD);
}

void Win32Input::OnFocusLost()
{
    ClearAllSources();
    Queue.push_back(InputEvent::FocusLost());
    MouseDeltaRemainderX = 0.0f;
    MouseDeltaRemainderY = 0.0f;
}

bool Win32Input::PollNext(InputEvent& out)
{
    if (Queue.empty())
    {
        return false;
    }
    out = Queue.front();
    Queue.pop_front();
    return true;
}
