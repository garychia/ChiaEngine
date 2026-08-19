#ifndef AUDIOSYSTEM_HPP
#define AUDIOSYSTEM_HPP

#include <cstdint>
#include <mutex>

#include "Data/HashTable.hpp"
#include "System/Audio/IAudioSystem.hpp"

// miniaudio 後端實作。miniaudio.h(95k 行)只在 .cpp 內含入並定義
// MA_IMPLEMENTATION,這裡只前向宣告 ma_engine,避免把整個庫塞進每個 TU。
struct ma_engine;

// 一條已解碼的 PCM buffer(內容定址快取)。
struct AudioBuffer
{
    uint32_t id;
    char path[512];
    // 解碼後的 PCM 資料(miniaudio 用 f32 交錯格式存放):
    float *pPcm;
    uint64_t pcmFrameCount;
    uint32_t channels;
    uint32_t sampleRate;
    uint32_t refCount; // 指向這份 buffer 的 sound 實例數 + 外部持有數
    bool loaded;
};

// 一條播放實例:指向某個 AudioBuffer + miniaudio 的 ma_sound。
struct AudioSound
{
    uint32_t id;
    uint32_t bufferId;
    ma_engine *pEngine; // 借用 AudioSystem 的 engine,不擁有
    void *pMaSound;     // ma_sound*,在 .cpp 內轉型
    void *pMaBuffer;    // ma_audio_buffer*,在 .cpp 內轉型(ma_sound 不接管,手動 uninit)
    bool inUse;
};

class AudioSystem : public IAudioSystem
{
  public:
    AudioSystem();
    ~AudioSystem() override;

    bool Initialize() override;
    void Shutdown() override;

    uint32_t LoadSound(const char *path) override;
    uint32_t Play(uint32_t buffer, bool loop = false) override;
    void Stop(uint32_t sound) override;
    void Pause(uint32_t sound) override;
    void Resume(uint32_t sound) override;
    void SetSoundVolume(uint32_t sound, float volume) override;
    void SetMasterVolume(float volume) override;
    void ReleaseBuffer(uint32_t buffer) override;

    uint32_t GetBufferCount() const override;
    uint32_t GetActiveSoundCount() const override;

  private:
    // 內部:把 WAV bytes 解碼成 f32 PCM,填入 out。成功回傳 true。
    bool DecodeWavToFloat(const unsigned char *bytes, size_t size, AudioBuffer &out);

    ma_engine *m_pEngine;
    bool m_initialized;
    mutable std::mutex m_mutex;

    HashTable<uint32_t, AudioBuffer *> m_buffers;  // id -> buffer
    HashTable<uint32_t, AudioSound *> m_sounds;     // id -> sound
    uint32_t m_nextBufferId;
    uint32_t m_nextSoundId;
};

#endif // AUDIOSYSTEM_HPP
