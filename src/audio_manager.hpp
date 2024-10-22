#pragma once
#include <opusfile.h>
#include <rtaudio/RtAudio.h>
#include "utils.hpp"
#include <vector>
#include <memory>

struct SoundEffect
{
    OggOpusFile *file;
    bool isPlaying;
    bool isLooping;
    float volume;

    SoundEffect(OggOpusFile *file = nullptr, bool isPlaying = true, bool isLooping = false, float volume = 1.f)
        : file(file), isPlaying(isPlaying), isLooping(isLooping), volume(volume) {}

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

    std::vector<std::shared_ptr<SoundEffect>> sounds;

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

    static int AudioCallback(void *outputBuffer, void * /*inputBuffer*/, uint32_t nBufferFrames,
                             double /*streamTime*/, RtAudioStreamStatus status, void * /*userData*/)
    {
        if (status)
            throw std::runtime_error(std::format("Stream underflow detected: {}", status));

        float *output = static_cast<float *>(outputBuffer);
        std::fill(output, output + nBufferFrames * CHANNELS, 0.f);

        AudioManager &audioManager = GetInstance();

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
                output[i] += tempBuffer[i] * sound->volume;
            }

            if (samplesRead == 0 && sound->isLooping)
                op_raw_seek(sound->file, 0);

            if (samplesRead == 0 && !sound->isLooping)
                sound->isPlaying = false;
        }

        return 0;
    }

public:
    static void LoadSoundEffect(const std::string &filePath, bool loop = false, float volume = 1.f)
    {
        AudioManager &audio = GetInstance();

        int error;
        OggOpusFile *soundFile = op_open_file(filePath.c_str(), &error);
        if (!soundFile)
            throw std::runtime_error(std::format("Error opening Opus file: {}", error));

        audio.sounds.push_back(std::make_shared<SoundEffect>(soundFile, true, loop, volume));
    }

    static void PauseSoundEffect(size_t soundIndex)
    {
        AudioManager &audio = GetInstance();
        if (soundIndex < audio.sounds.size())
        {
            audio.sounds[soundIndex]->Pause();
        }
    }

    static void PlaySoundEffect(size_t soundIndex)
    {
        AudioManager &audio = GetInstance();
        if (soundIndex < audio.sounds.size())
        {
            audio.sounds[soundIndex]->Play();
        }
    }

    static void StopSoundEffect(size_t soundIndex)
    {
        AudioManager &audio = GetInstance();
        if (soundIndex < audio.sounds.size())
        {
            audio.sounds[soundIndex]->Stop();
        }
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
