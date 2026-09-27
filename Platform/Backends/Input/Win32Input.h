#pragma once

#include "IInput.h"

#include <cstdint>
#include <deque>

class Win32Input final : public IInput
{
public:
    Win32Input();
    ~Win32Input() override;

    bool Init() override;
    void Destroy() override;

    void OnKeyEvent(int nativeKeyCode, bool isDown, bool isRepeat) override;
    void OnMouseMovement(float x, float y, float xrel, float yrel) override;
    void OnMouseButton(int button, bool isDown, float x, float y) override;
    void OnGamepadConnected(int instanceId) override;
    void OnGamepadDisconnected(int instanceId) override;
    void OnGamepadButton(int instanceId, int button, bool isDown) override;
    void OnGamepadAxis(int instanceId, int axis, int value) override;
    void OnFocusLost() override;

    bool PollNext(InputEvent& out) override;

private:
    static int TranslateKey(int nativeKeyCode);
    static int TranslateGamepadButton(int button);
    static void TranslateGamepadAxis(int axis, int& outNegativeCode, int& outPositiveCode);

    void SetSource(uint32_t sourceId, int code, bool isDown);
    void ClearAllSources();

    float MouseDeltaRemainderX = 0.0f;
    float MouseDeltaRemainderY = 0.0f;

    std::deque<InputEvent> Queue;
};
