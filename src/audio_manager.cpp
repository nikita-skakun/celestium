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
    catch (const RtAudioError &e)
    {
        LogMessage(LogLevel::ERROR, std::format("Error stopping audio stream: {}", e.getMessage()));
    }

    try
    {
        if (audioStream.isStreamOpen())
            audioStream.closeStream();
    }
    catch (const RtAudioError &e)
    {
        LogMessage(LogLevel::ERROR, std::format("Error closing audio stream: {}", e.getMessage()));
    }

    for (auto &sound : audio.sounds)
    {
        if (sound->file)
            op_free(sound->file);
    }
    std::vector<std::shared_ptr<SoundEffect>>().swap(audio.sounds);
}