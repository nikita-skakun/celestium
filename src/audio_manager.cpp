#include "audio_manager.hpp"

void AudioManager::CleanUp()
{
    AudioManager &audio = GetInstance();
    auto &audioStream = audio.audioStream;

    try
    {
        if (audioStream.isStreamRunning())
            audioStream.stopStream();
    }
    catch (const RtAudioErrorType &e)
    {
        TraceLog(TraceLogLevel::LOG_ERROR, std::format("Error stopping audio stream: {}", static_cast<int>(e)).c_str());
    }

    try
    {
        if (audioStream.isStreamOpen())
            audioStream.closeStream();
    }
    catch (const RtAudioErrorType &e)
    {
        TraceLog(TraceLogLevel::LOG_ERROR, std::format("Error closing audio stream: {}", static_cast<int>(e)).c_str());
    }

    for (auto &sound : audio.sounds)
    {
        if (sound->file)
            op_free(sound->file);
    }
    audio.sounds.clear();
}