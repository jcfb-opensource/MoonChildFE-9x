#pragma once

#include "IAudio.h"

class WinMMAudio final : public IAudio
{
public:
    WinMMAudio();
    ~WinMMAudio() override;

    bool Init() override;
    void Destroy() override;
    void Update() override;

    SoundHandle CreateSound(int soundId, int maxPolyphony) override;
    void DestroySound(SoundHandle sound) override;

    void PlayOneShot(SoundHandle sound, int32_t volume, int32_t pan) override;
    void PlayLoop(SoundHandle sound, int32_t volume, int32_t pan) override;

    void StopSound(SoundHandle sound) override;
    void StopCurrent(SoundHandle sound) override;

    void SetVolume(SoundHandle sound, int32_t volume) override;
    void SetPan(SoundHandle sound, int32_t pan) override;

    void Reset() override;

    bool PlayMusic(int trackNumber) override;
    void StopMusic() override;

    bool OpenMovieStream(int sampleRate, int channels) override;
    void SubmitMovieAudio(const float* samples, int sampleCount) override;
    void CloseMovieStream() override;
};
