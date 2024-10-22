#pragma once
#include <opusfile.h>
#include <rtaudio/RtAudio.h>
#include "utils.hpp"

struct SoundEffect
{
    OggOpusFile *file;
    bool isPlaying; // TODO: Implement play/pause/stop functionality
    bool isLooping; // TODO: Implement looping functionality

    SoundEffect(OggOpusFile *file = nullptr, bool isPlaying = true, bool isLooping = false)
        : file(file), isPlaying(isPlaying), isLooping(isLooping) {}
};

struct AudioManager
{
private:
    RtAudio audioStream = RtAudio(RtAudio::Api::LINUX_PULSE);
    RtAudio::StreamParameters outputParams;
    RtAudio::StreamOptions options;
    uint32_t bufferFrames = 32;

    // TODO: Load multiple sound files
    SoundEffect sound = SoundEffect();

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

    static int AudioCallback(void *outputBuffer, void * /*inputBuffer*/, unsigned int nBufferFrames,
                             double /*streamTime*/, RtAudioStreamStatus status, void * /*userData*/)
    {
        if (status)
            throw std::runtime_error("Stream overflow detected!");

        // TODO: Accept more then just one sound file
        auto file = GetInstance().sound.file;
        float *buffer = static_cast<float *>(outputBuffer);

        unsigned int samplesRead = op_read_float(file, buffer, nBufferFrames * CHANNELS, nullptr);

        if (samplesRead < 0)
            throw std::runtime_error("Error reading Opus file!");

        // Fill the remaining buffer with silence
        if (samplesRead < nBufferFrames)
            std::fill(buffer + samplesRead * CHANNELS, buffer + nBufferFrames * CHANNELS, 0.f);

        return (int)(samplesRead == 0);
    }

public:
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

        int error;
        OggOpusFile *fireAlarmSound = op_open_file("../assets/audio/fire_alarm.opus", &error);
        if (!fireAlarmSound)
            throw std::runtime_error(std::format("Error opening Opus file: {}", error));

        audio.sound = SoundEffect(fireAlarmSound, true, true);

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

        op_free(audio.sound.file);
        audio.sound.file = nullptr;
    };
};