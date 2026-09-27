#include "WinMMAudio.h"

#include "AudioEngine.h"
#include "MediaCatalog.h"
#include "MP3MusicTrack.h"

#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#pragma comment(lib, "winmm.lib")

extern const char* FullPath(const char* file);

static constexpr int MIX_FREQUENCY = 44100;
static constexpr int MIX_CHANNELS = 2;
static constexpr int BUFFER_FRAMES = 4096;
static constexpr int BUFFER_SAMPLES = BUFFER_FRAMES * MIX_CHANNELS;
// This is single-threaded (see the CALLBACK_EVENT design below) and only
// serviced when Update() is polled from the main loop, so a synchronous stall
// on that thread (e.g. loading assets for a new menu) can starve playback if
// the buffered-ahead audio isn't deep enough. 8 * 4096 frames @ 44100Hz is
// ~745ms of buffered audio, which comfortably absorbs that kind of hitch at
// the cost of a bit of extra output latency (fine for background music/sfx).
static constexpr int NUM_BUFFERS = 8;
static constexpr int MUSIC_DECODE_BATCH_FRAMES = 4096;

// A tiny linear-interpolation resampler that also handles mono -> stereo upmixing.
// Good enough fidelity for a Windows 9x era port without pulling in a 3rd party DSP library.
struct StreamResampler
{
    std::vector<float> Backlog;
    double Pos = 0.0;
    int SrcChannels = 2;
    int SrcRate = MIX_FREQUENCY;

    void SetFormat(int channels, int rate)
    {
        SrcChannels = std::max(1, channels);
        SrcRate = std::max(1, rate);
        Backlog.clear();
        Pos = 0.0;
    }

    void Push(const float* interleaved, int frameCount)
    {
        if (frameCount <= 0)
        {
            return;
        }
        Backlog.insert(Backlog.end(), interleaved, interleaved + static_cast<size_t>(frameCount) * SrcChannels);
    }

    int Pull(float* dstInterleavedStereo, int dstFrameCount, int dstRate)
    {
        if (SrcRate <= 0 || SrcChannels <= 0 || Backlog.empty())
        {
            return 0;
        }

        const double step = static_cast<double>(SrcRate) / static_cast<double>(dstRate);
        const size_t srcFrameCountAvail = Backlog.size() / static_cast<size_t>(SrcChannels);
        int produced = 0;

        for (; produced < dstFrameCount; produced++)
        {
            const size_t idx0 = static_cast<size_t>(Pos);
            if (idx0 + 1 >= srcFrameCountAvail)
            {
                break;
            }

            const double frac = Pos - static_cast<double>(idx0);
            for (int ch = 0; ch < 2; ch++)
            {
                const int srcCh = (SrcChannels == 1) ? 0 : std::min(ch, SrcChannels - 1);
                const float a = Backlog[idx0 * SrcChannels + srcCh];
                const float b = Backlog[(idx0 + 1) * SrcChannels + srcCh];
                dstInterleavedStereo[produced * 2 + ch] = a + static_cast<float>((b - a) * frac);
            }
            Pos += step;
        }

        const size_t consumedFrames = static_cast<size_t>(Pos);
        if (consumedFrames > 0 && consumedFrames <= srcFrameCountAvail)
        {
            Backlog.erase(Backlog.begin(), Backlog.begin() + static_cast<std::ptrdiff_t>(consumedFrames * SrcChannels));
            Pos -= static_cast<double>(consumedFrames);
        }

        return produced;
    }
};

struct SfxState
{
    HWAVEOUT Device = nullptr;
    WAVEHDR Headers[NUM_BUFFERS] = {};
    std::vector<short> Buffers[NUM_BUFFERS];
    bool Prepared[NUM_BUFFERS] = {};
    bool Ready = false;

    // Opened with CALLBACK_EVENT rather than CALLBACK_FUNCTION + a worker thread:
    // the WOM_DONE callback is only allowed to call a small set of documented-safe
    // functions (waveOutWrite/Prepare/UnprepareHeader are NOT in that list), and a
    // second thread turned out to be unreliable on real Windows 98 too. Instead,
    // Windows just signals this event and WinMMAudio::Update() (called every host
    // frame from the main thread) drains any buffers marked WHDR_DONE.
    HANDLE RefillEvent = nullptr;
};

struct MusicState
{
    MP3MusicTrack Source;
    StreamResampler Resampler;
    float Gain = 1.0f;
    bool Active = false;
};

struct MovieState
{
    StreamResampler Resampler;
    bool Active = false;
};

static SfxState Sfx;
static MusicState Music;
static MovieState Movie;
static AudioEngine Audio;
static CRITICAL_SECTION AudioLock;
static bool AudioLockInitialized = false;

static float MixBuffer[BUFFER_SAMPLES];
static float MusicDecodeScratch[MUSIC_DECODE_BATCH_FRAMES * 2];
static float MixedAudioScratch[BUFFER_SAMPLES];

static void LockAudio()
{
    if (AudioLockInitialized)
    {
        EnterCriticalSection(&AudioLock);
    }
}

static void UnlockAudio()
{
    if (AudioLockInitialized)
    {
        LeaveCriticalSection(&AudioLock);
    }
}

static bool LoadWav(const char* path, std::vector<float>& outSamples, int& outFrameCount)
{
    FILE* file = fopen(path, "rb");
    if (file == nullptr)
    {
        printf("Failed to open WAV file! %s\n", path);
        return false;
    }

    char riffId[4];
    uint32_t riffSize = 0;
    char waveId[4];
    if (fread(riffId, 1, 4, file) != 4 || memcmp(riffId, "RIFF", 4) != 0 ||
        fread(&riffSize, 4, 1, file) != 1 ||
        fread(waveId, 1, 4, file) != 4 || memcmp(waveId, "WAVE", 4) != 0)
    {
        printf("Not a valid WAV file! %s\n", path);
        fclose(file);
        return false;
    }

    WAVEFORMATEX format = {};
    bool haveFormat = false;
    std::vector<unsigned char> dataChunk;

    while (true)
    {
        char chunkId[4];
        uint32_t chunkSize = 0;
        if (fread(chunkId, 1, 4, file) != 4 || fread(&chunkSize, 4, 1, file) != 1)
        {
            break;
        }

        if (memcmp(chunkId, "fmt ", 4) == 0)
        {
            std::vector<unsigned char> buffer(chunkSize);
            if (fread(buffer.data(), 1, chunkSize, file) != chunkSize)
            {
                break;
            }
            memcpy(&format, buffer.data(), std::min<size_t>(sizeof(format), buffer.size()));
            haveFormat = true;
        }
        else if (memcmp(chunkId, "data", 4) == 0)
        {
            dataChunk.resize(chunkSize);
            if (chunkSize > 0 && fread(dataChunk.data(), 1, chunkSize, file) != chunkSize)
            {
                break;
            }
        }
        else
        {
            fseek(file, static_cast<long>(chunkSize), SEEK_CUR);
        }

        if ((chunkSize % 2) == 1)
        {
            fseek(file, 1, SEEK_CUR);
        }
    }

    fclose(file);

    if (!haveFormat || dataChunk.empty() || format.nChannels == 0)
    {
        printf("WAV file missing fmt/data chunks! %s\n", path);
        return false;
    }

    const int bits = format.wBitsPerSample;
    const int channels = format.nChannels;
    const size_t bytesPerSample = static_cast<size_t>(bits) / 8;
    const size_t frameCount = bytesPerSample > 0 ? dataChunk.size() / (bytesPerSample * channels) : 0;

    std::vector<float> decoded(frameCount * channels);

    if (format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT && bits == 32)
    {
        const float* src = reinterpret_cast<const float*>(dataChunk.data());
        std::memcpy(decoded.data(), src, decoded.size() * sizeof(float));
    }
    else if (format.wFormatTag == WAVE_FORMAT_PCM && bits == 16)
    {
        const int16_t* src = reinterpret_cast<const int16_t*>(dataChunk.data());
        for (size_t i = 0; i < decoded.size(); i++)
        {
            decoded[i] = src[i] / 32768.0f;
        }
    }
    else if (format.wFormatTag == WAVE_FORMAT_PCM && bits == 8)
    {
        const uint8_t* src = dataChunk.data();
        for (size_t i = 0; i < decoded.size(); i++)
        {
            decoded[i] = (static_cast<int>(src[i]) - 128) / 128.0f;
        }
    }
    else if (format.wFormatTag == WAVE_FORMAT_PCM && bits == 24)
    {
        const uint8_t* src = dataChunk.data();
        for (size_t i = 0; i < decoded.size(); i++)
        {
            const int32_t sample = (static_cast<int32_t>(src[i * 3 + 2]) << 24 |
                                     static_cast<int32_t>(src[i * 3 + 1]) << 16 |
                                     static_cast<int32_t>(src[i * 3 + 0]) << 8) >> 8;
            decoded[i] = sample / 8388608.0f;
        }
    }
    else
    {
        printf("Unsupported WAV format! %s (tag=%d bits=%d)\n", path, format.wFormatTag, bits);
        return false;
    }

    StreamResampler resampler;
    resampler.SetFormat(channels, static_cast<int>(format.nSamplesPerSec));
    resampler.Push(decoded.data(), static_cast<int>(frameCount));

    outSamples.clear();
    std::vector<float> scratch(2048 * 2);
    while (true)
    {
        const int got = resampler.Pull(scratch.data(), 2048, MIX_FREQUENCY);
        if (got <= 0)
        {
            break;
        }
        outSamples.insert(outSamples.end(), scratch.begin(), scratch.begin() + static_cast<size_t>(got) * 2);
    }

    outFrameCount = static_cast<int>(outSamples.size() / 2);
    return outFrameCount > 0;
}

static void CleanUpMusicStuff(MusicState& musicState)
{
    musicState.Source.Close();
    musicState.Resampler = StreamResampler();
    musicState.Active = false;
}

static void FeedMusicResampler(int neededFrames)
{
    if (!Music.Active || !Music.Source.IsOpen())
    {
        return;
    }

    const int channels = std::max(1, Music.Source.GetChannelCount());
    const double step = static_cast<double>(Music.Source.GetSampleRate()) / static_cast<double>(MIX_FREQUENCY);
    const size_t neededSrcFrames = static_cast<size_t>(neededFrames * step) + 4;

    // MP3MusicTrack::ReadFrames loops the track internally on EOF, so it (almost)
    // never returns 0 - the available-frame count must be re-checked every
    // iteration, otherwise this becomes an infinite loop that never returns.
    while ((Music.Resampler.Backlog.size() / static_cast<size_t>(channels)) < neededSrcFrames)
    {
        const size_t framesRead = Music.Source.ReadFrames(MusicDecodeScratch, MUSIC_DECODE_BATCH_FRAMES);
        if (framesRead == 0)
        {
            break;
        }
        Music.Resampler.Push(MusicDecodeScratch, static_cast<int>(framesRead));
        if (framesRead < static_cast<size_t>(MUSIC_DECODE_BATCH_FRAMES))
        {
            break;
        }
    }
}

static void MixMusic(float* dst, int sampleCount)
{
    if (!Music.Active)
    {
        return;
    }

    const int frameCount = sampleCount / MIX_CHANNELS;
    FeedMusicResampler(frameCount);

    const int got = Music.Resampler.Pull(MixedAudioScratch, frameCount, MIX_FREQUENCY);
    const float gain = Music.Gain;
    for (int i = 0; i < got * MIX_CHANNELS; i++)
    {
        dst[i] += MixedAudioScratch[i] * gain;
    }
}

static void MixMovie(float* dst, int sampleCount)
{
    if (!Movie.Active)
    {
        return;
    }

    const int frameCount = sampleCount / MIX_CHANNELS;
    const int got = Movie.Resampler.Pull(MixedAudioScratch, frameCount, MIX_FREQUENCY);
    for (int i = 0; i < got * MIX_CHANNELS; i++)
    {
        dst[i] += MixedAudioScratch[i];
    }
}

static void FillMixBuffer(short* out, int sampleCount)
{
    std::memset(MixBuffer, 0, static_cast<size_t>(sampleCount) * sizeof(float));

    LockAudio();
    MixMusic(MixBuffer, sampleCount);
    MixMovie(MixBuffer, sampleCount);
    Audio.MixVoicesInto(MixBuffer, sampleCount);
    UnlockAudio();

    for (int i = 0; i < sampleCount; i++)
    {
        float sample = MixBuffer[i];
        sample = std::max(-1.0f, std::min(1.0f, sample));
        out[i] = static_cast<short>(sample * 32767.0f);
    }
}

static void QueueBuffer(int index)
{
    Sfx.Buffers[index].resize(BUFFER_SAMPLES);
    FillMixBuffer(Sfx.Buffers[index].data(), BUFFER_SAMPLES);

    WAVEHDR& header = Sfx.Headers[index];
    if (Sfx.Prepared[index])
    {
        waveOutUnprepareHeader(Sfx.Device, &header, sizeof(WAVEHDR));
    }

    ZeroMemory(&header, sizeof(WAVEHDR));
    header.lpData = reinterpret_cast<LPSTR>(Sfx.Buffers[index].data());
    header.dwBufferLength = static_cast<DWORD>(Sfx.Buffers[index].size() * sizeof(short));

    waveOutPrepareHeader(Sfx.Device, &header, sizeof(WAVEHDR));
    Sfx.Prepared[index] = true;
    waveOutWrite(Sfx.Device, &header, sizeof(WAVEHDR));
}

static void ServicePendingBuffers()
{
    if (!Sfx.Ready || Sfx.RefillEvent == nullptr)
    {
        return;
    }

    if (WaitForSingleObject(Sfx.RefillEvent, 0) != WAIT_OBJECT_0)
    {
        return;
    }
    ResetEvent(Sfx.RefillEvent);

    for (int i = 0; i < NUM_BUFFERS; i++)
    {
        if (Sfx.Prepared[i] && (Sfx.Headers[i].dwFlags & WHDR_DONE) != 0)
        {
            QueueBuffer(i);
        }
    }
}

WinMMAudio::WinMMAudio() = default;

WinMMAudio::~WinMMAudio()
{
    Destroy();
}

bool WinMMAudio::Init()
{
    if (Sfx.Ready)
    {
        return true;
    }

    if (!AudioLockInitialized)
    {
        InitializeCriticalSection(&AudioLock);
        AudioLockInitialized = true;
    }

    WAVEFORMATEX format = {};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = MIX_CHANNELS;
    format.nSamplesPerSec = MIX_FREQUENCY;
    format.wBitsPerSample = 16;
    format.nBlockAlign = static_cast<WORD>(format.nChannels * format.wBitsPerSample / 8);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

    Sfx.RefillEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    if (Sfx.RefillEvent == nullptr)
    {
        printf("CreateEventA failed! Error %lu\n", GetLastError());
        return false;
    }

    const MMRESULT result = waveOutOpen(&Sfx.Device, WAVE_MAPPER, &format,
        reinterpret_cast<DWORD_PTR>(Sfx.RefillEvent), 0, CALLBACK_EVENT);
    if (result != MMSYSERR_NOERROR)
    {
        printf("waveOutOpen failed! Error %d\n", result);
        CloseHandle(Sfx.RefillEvent);
        Sfx.RefillEvent = nullptr;
        return false;
    }

    for (int i = 0; i < NUM_BUFFERS; i++)
    {
        QueueBuffer(i);
    }

    Sfx.Ready = true;
    return true;
}

void WinMMAudio::Destroy()
{
    StopMusic();
    Reset();

    if (Sfx.Device != nullptr)
    {
        waveOutReset(Sfx.Device);
        for (int i = 0; i < NUM_BUFFERS; i++)
        {
            if (Sfx.Prepared[i])
            {
                waveOutUnprepareHeader(Sfx.Device, &Sfx.Headers[i], sizeof(WAVEHDR));
                Sfx.Prepared[i] = false;
            }
        }
        waveOutClose(Sfx.Device);
        Sfx.Device = nullptr;
    }

    if (Sfx.RefillEvent != nullptr)
    {
        CloseHandle(Sfx.RefillEvent);
        Sfx.RefillEvent = nullptr;
    }

    Sfx.Ready = false;
}

void WinMMAudio::Update()
{
    ServicePendingBuffers();
}

SoundHandle WinMMAudio::CreateSound(int soundId, int maxPolyphony)
{
    if (!Init())
    {
        return 0;
    }
    return Audio.CreateSound(soundId, maxPolyphony, LoadWav);
}

void WinMMAudio::DestroySound(SoundHandle sound)
{
    LockAudio();
    Audio.DestroySound(sound);
    UnlockAudio();
}

void WinMMAudio::PlayOneShot(SoundHandle sound, int32_t volume, int32_t pan)
{
    LockAudio();
    Audio.PlayOneShot(sound, volume, pan);
    UnlockAudio();
}

void WinMMAudio::PlayLoop(SoundHandle sound, int32_t volume, int32_t pan)
{
    LockAudio();
    Audio.PlayLoop(sound, volume, pan);
    UnlockAudio();
}

void WinMMAudio::StopSound(SoundHandle sound)
{
    LockAudio();
    Audio.StopSound(sound);
    UnlockAudio();
}

void WinMMAudio::StopCurrent(SoundHandle sound)
{
    LockAudio();
    Audio.StopCurrent(sound);
    UnlockAudio();
}

void WinMMAudio::SetVolume(SoundHandle sound, int32_t volume)
{
    LockAudio();
    Audio.SetVolume(sound, volume);
    UnlockAudio();
}

void WinMMAudio::SetPan(SoundHandle sound, int32_t pan)
{
    LockAudio();
    Audio.SetPan(sound, pan);
    UnlockAudio();
}

void WinMMAudio::Reset()
{
    CloseMovieStream();
    LockAudio();
    Audio.Reset();
    UnlockAudio();
}

bool WinMMAudio::PlayMusic(int trackNumber)
{
    const MusicTrack* track = GetMusicTrack(trackNumber);
    if (track == nullptr)
    {
        return false;
    }

    if (!Init())
    {
        return false;
    }

    StopMusic();

    char relBuf[512];
    std::snprintf(relBuf, sizeof(relBuf), "mp3/%s", track->Path);
    const char* path = FullPath(relBuf);

    if (!Music.Source.Open(path))
    {
        printf("Opening MP3MusicTrack failed! %s\n", path);
        return false;
    }

    LockAudio();
    Music.Resampler.SetFormat(Music.Source.GetChannelCount(), Music.Source.GetSampleRate());
    Music.Gain = track->Volume;
    Music.Active = true;
    UnlockAudio();

    return true;
}

void WinMMAudio::StopMusic()
{
    LockAudio();
    CleanUpMusicStuff(Music);
    UnlockAudio();
}

bool WinMMAudio::OpenMovieStream(int sampleRate, int channels)
{
    CloseMovieStream();

    if (sampleRate <= 0 || channels <= 0 || channels > 2)
    {
        return false;
    }

    if (!Init())
    {
        return false;
    }

    LockAudio();
    Movie.Resampler.SetFormat(channels, sampleRate);
    Movie.Active = true;
    UnlockAudio();

    return true;
}

void WinMMAudio::SubmitMovieAudio(const float* samples, int sampleCount)
{
    if (!Movie.Active || sampleCount <= 0)
    {
        return;
    }

    LockAudio();
    Movie.Resampler.Push(samples, sampleCount / std::max(1, Movie.Resampler.SrcChannels));
    UnlockAudio();
}

void WinMMAudio::CloseMovieStream()
{
    LockAudio();
    Movie.Active = false;
    Movie.Resampler = StreamResampler();
    UnlockAudio();
}
