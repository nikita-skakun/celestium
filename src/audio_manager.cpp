#include "audio_manager.hpp"
#include "logging.hpp"

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
        LogMessage(LogLevel::ERROR, std::format("Error stopping audio stream: {}", static_cast<int>(e)));
    }

    try
    {
        if (audioStream.isStreamOpen())
            audioStream.closeStream();
    }
    catch (const RtAudioErrorType &e)
    {
        LogMessage(LogLevel::ERROR, std::format("Error closing audio stream: {}", static_cast<int>(e)));
    }

    for (auto &sound : audio.sounds)
    {
        if (sound->file)
            op_free(sound->file);
    }
    audio.sounds.clear();
}