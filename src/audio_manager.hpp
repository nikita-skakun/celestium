#pragma once
#include "utils.hpp"
#include <memory>
#include <opus/opusfile.h>
#include <rtaudio/RtAudio.h>
#include <unordered_set>

struct SoundEffect
{
    enum class Type
    {
        MUSIC,
        EFFECT
    };

    OggOpusFile *file;
    Type type;
    bool isPlaying;
    bool isLooping;
    float volume;
    std::function<bool()> onUpdate;

    SoundEffect(OggOpusFile *file, Type type, bool isPlaying = true, bool isLooping = false, float volume = 1.f, std::function<bool()> onUpdate = nullptr)
        : file(file), type(type), isPlaying(isPlaying), isLooping(isLooping), volume(volume), onUpdate(onUpdate) {}

    void Play() { isPlaying = true; }
    void Pause() { isPlaying = false; }
    void Stop()
    {
        isPlaying = false;
        op_raw_seek(file, 0);
    }
};

struct AudioManager
{
private:
    RtAudio audioStream = RtAudio(RtAudio::Api::LINUX_PULSE);
    RtAudio::StreamParameters outputParams;
    RtAudio::StreamOptions options;
    uint32_t bufferFrames = 32;

    std::unordered_set<std::shared_ptr<SoundEffect>> sounds;

    float masterVolume = 1.f;
    float musicVolume = 1.f;
    float effectsVolume = 1.f;

    AudioManager() = default;
    ~AudioManager() = default;
    AudioManager(const AudioManager &) = delete;
    AudioManager &operator=(const AudioManager &) = delete;

    static constexpr uint8_t CHANNELS = 2;
    static constexpr uint32_t SAMPLE_RATE = 48000;

    static AudioManager &GetInstance()
    {
        static AudioManager instance;
        return instance;
    }

    static float GetVolume(SoundEffect::Type type)
    {
        AudioManager &audio = GetInstance();
        float volume = audio.masterVolume;

        switch (type)
        {
        case SoundEffect::Type::MUSIC:
            return volume * audio.musicVolume;

        case SoundEffect::Type::EFFECT:
            return volume * audio.effectsVolume;

        default:
            break;
        }

        return volume;
    }

    static int AudioCallback(void *outputBuffer, void * /*inputBuffer*/, uint32_t nBufferFrames,
                             double /*streamTime*/, RtAudioStreamStatus status, void * /*userData*/)
    {
        if (status)
            throw std::runtime_error(std::format("Stream underflow detected: {}", status));

        float *output = static_cast<float *>(outputBuffer);
        AudioManager &audioManager = GetInstance();

        if (audioManager.sounds.empty())
            return 0;

        std::fill(output, output + nBufferFrames * CHANNELS, 0.f);
        std::vector<float> tempBuffer(nBufferFrames * CHANNELS);

        for (auto &sound : audioManager.sounds)
        {
            if (!sound->isPlaying)
                continue;

            std::fill(tempBuffer.begin(), tempBuffer.end(), 0.f);

            uint32_t samplesRead = op_read_float(sound->file, tempBuffer.data(), nBufferFrames * CHANNELS, nullptr);

            if (samplesRead < 0)
                throw std::runtime_error("Error reading Opus file!");

            for (uint32_t i = 0; i < samplesRead * CHANNELS; ++i)
            {
                output[i] += tempBuffer[i] * sound->volume * GetVolume(sound->type);
            }

            if (samplesRead == 0 && sound->isLooping)
                op_raw_seek(sound->file, 0);

            if (samplesRead == 0 && !sound->isLooping)
                sound->isPlaying = false;
        }

        for (uint32_t i = 0; i < nBufferFrames * CHANNELS; ++i)
        {
            output[i] = std::clamp(output[i], -1.f, 1.f);
        }

        return 0;
    }

public:
    static float GetMasterVolume() { return GetInstance().masterVolume; }
    static void SetMasterVolume(float volume) { GetInstance().masterVolume = std::clamp(volume, 0.f, 1.f); }
    static float GetMusicVolume() { return GetInstance().musicVolume; }
    static void SetMusicVolume(float volume) { GetInstance().musicVolume = std::clamp(volume, 0.f, 1.f); }
    static float GetEffectsVolume() { return GetInstance().effectsVolume; }
    static void SetEffectsVolume(float volume) { GetInstance().effectsVolume = std::clamp(volume, 0.f, 1.f); }

    static std::shared_ptr<SoundEffect> LoadSoundEffect(const std::string &filePath, SoundEffect::Type type, bool startPlaying = false, bool loop = false, float volume = 1.f)
    {
        AudioManager &audio = GetInstance();

        int error;
        OggOpusFile *soundFile = op_open_file(filePath.c_str(), &error);
        if (!soundFile)
            throw std::runtime_error(std::format("Error opening Opus file: {}", error));

        auto sound = std::make_shared<SoundEffect>(soundFile, type, startPlaying, loop, volume);
        audio.sounds.insert(sound);
        return sound;
    }

    static void Initialize()
    {
        AudioManager &audio = GetInstance();
        auto &audioStream = audio.audioStream;
        auto &outputParams = audio.outputParams;
        auto &options = audio.options;

        audioStream.showWarnings(true);

        if (audioStream.getDeviceCount() < 1)
            throw std::runtime_error("No audio devices found!");

        outputParams.deviceId = audioStream.getDefaultOutputDevice();
        outputParams.nChannels = CHANNELS;
        outputParams.firstChannel = 0;

        options.flags = RTAUDIO_MINIMIZE_LATENCY;

        if (audioStream.openStream(&outputParams, nullptr, RTAUDIO_FLOAT32, SAMPLE_RATE, &audio.bufferFrames, AudioCallback, nullptr, &options))
            throw std::runtime_error(std::format("Error opening stream: {}", audioStream.getErrorText()));

        if (audioStream.startStream())
            throw std::runtime_error(std::format("Error starting stream: {}", audioStream.getErrorText()));
    }

    static void Update()
    {
        AudioManager &audio = GetInstance();
        for (const auto &sound : audio.sounds)
        {
            if (!sound || (sound->onUpdate && sound->onUpdate()))
                audio.sounds.erase(sound);
        }
    }

    static void CleanUp()
    {
        AudioManager &audio = GetInstance();
        auto &audioStream = audio.audioStream;

        if (audioStream.isStreamRunning())
            audioStream.stopStream();
        if (audioStream.isStreamOpen())
            audioStream.closeStream();

        for (auto &sound : audio.sounds)
        {
            op_free(sound->file);
        }

        audio.sounds.clear();
    }
};
