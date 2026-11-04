#ifndef AUDIO_SOURCE_HPP
#define AUDIO_SOURCE_HPP

#include <cstdint>

// AudioSource:附著於 Entity 的音訊播放元件。與 IAudioSystem/AudioSystem(miniaudio)
// 的 SoundHandle 對應。本身不持有音訊裝置，只記錄播放狀態與參數，
// 由 AudioRuntime(SceneSystem 整合)在 Update 中分派到後端。
// trivially copyable、無 padding → 可安全進 ComponentPool::Hash。
struct AudioSource
{
    uint32_t bufferId = 0;     // 關聯的音訊緩衝 ID(0 = 無)
    uint32_t soundId = 0;      // 當前播放實例 ID(0 = 未播放)
    float volume = 1.0f;       // [0,1]
    bool loop = false;         // 是否循環
    bool playOnAwake = false;  // Awake 時自動播放
    bool isPlaying = false;    // 運行時狀態：是否正在播放
    bool isPaused = false;     // 運行時狀態：是否暫停
    bool spatial = false;      // 是否為 3D 空間音訊(需 AudioListener)

    AudioSource() = default;
    AudioSource(uint32_t bufferId_, float volume_ = 1.0f, bool loop_ = false,
                bool playOnAwake_ = false, bool spatial_ = false)
        : bufferId(bufferId_), volume(volume_), loop(loop_),
          playOnAwake(playOnAwake_), spatial(spatial_)
    {
    }
};

#endif // AUDIO_SOURCE_HPP
