#ifndef CHIAENGINE_AUDIO_IAUDIOSYSTEM_HPP
#define CHIAENGINE_AUDIO_IAUDIOSYSTEM_HPP

#include <string>
#include <vector>
#include <memory>

namespace ChiaEngine {
namespace System {
namespace Audio {

/**
 * @brief Audio system interface for playing sound effects and music.
 */
class IAudioSystem {
public:
    virtual ~IAudioSystem() = default;

    /**
     * @brief Initialize the audio system.
     * @return True if initialization succeeded, false otherwise.
     */
    virtual bool Initialize() = 0;

    /**
     * @brief Shutdown the audio system.
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Load a sound effect from a file.
     * @param filePath Path to the sound effect file.
     * @return A handle to the loaded sound effect, or nullptr if loading failed.
     */
    virtual std::shared_ptr<void> LoadSoundEffect(const std::string& filePath) = 0;

    /**
     * @brief Play a sound effect.
     * @param soundHandle Handle to the sound effect to play.
     * @param loop Whether to loop the sound effect (default: false).
     * @param volume Volume level (0.0 to 1.0, default: 1.0).
     */
    virtual void PlaySoundEffect(std::shared_ptr<void> soundHandle, bool loop = false, float volume = 1.0f) = 0;

    /**
     * @brief Stop a playing sound effect.
     * @param soundHandle Handle to the sound effect to stop.
     */
    virtual void StopSoundEffect(std::shared_ptr<void> soundHandle) = 0;

    /**
     * @brief Load music from a file.
     * @param filePath Path to the music file.
     * @return A handle to the loaded music, or nullptr if loading failed.
     */
    virtual std::shared_ptr<void> LoadMusic(const std::string& filePath) = 0;

    /**
     * @brief Play music.
     * @param musicHandle Handle to the music to play.
     * @param loop Whether to loop the music (default: true).
     * @param volume Volume level (0.0 to 1.0, default: 1.0).
     */
    virtual void PlayMusic(std::shared_ptr<void> musicHandle, bool loop = true, float volume = 1.0f) = 0;

    /**
     * @brief Stop the currently playing music.
     */
    virtual void StopMusic() = 0;

    /**
     * @brief Pause the currently playing music.
     */
    virtual void PauseMusic() = 0;

    /**
     * @brief Resume the currently paused music.
     */
    virtual void ResumeMusic() = 0;

    /**
     * @brief Set the master volume for the audio system.
     * @param volume Volume level (0.0 to 1.0).
     */
    virtual void SetMasterVolume(float volume) = 0;

    /**
     * @brief Get the current master volume.
     * @return Current master volume (0.0 to 1.0).
     */
    virtual float GetMasterVolume() const = 0;
};

} // namespace Audio
} // namespace System
} // namespace ChiaEngine

#endif // CHIAENGINE_AUDIO_IAUDIOSYSTEM_HPP