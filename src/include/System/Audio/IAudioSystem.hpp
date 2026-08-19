#ifndef IAUDIOSYSTEM_HPP
#define IAUDIOSYSTEM_HPP

#include <cstdint>

// 音訊子系統抽象介面 — 平台無關的服務(principle #4:透過 EngineContext 註冊/解析)。
//
// 實作層(目前唯一一個)是 miniaudio 後端(AudioSystem),負責真正的裝置初始化
// 與 PCM 播放;但介面本身不依賴 miniaudio,也不碰任何 GPU/顯示型別。
//
// 資源模型(內容定址,複用 AssetManager 的 #39 契約):
//  - LoadSound(path) 以「檔案路徑」為 key,透過 AssetManager 讀出 WAV bytes,
//    解碼成 PCM 後以 path 為 key 快取。相同路徑載入兩次 → 同一份 AudioBuffer。
//  - Play(buffer) 在 buffer 上建立一條播放實例(SoundHandle),可同時多條重疊。
//  - 釋放順序由呼叫端用 SoundHandle / AudioBuffer 的 Release 管理。
class IAudioSystem
{
  public:
    virtual ~IAudioSystem() = default;

    // 初始化後端(建立 miniaudio engine / 裝置)。
    // 失敗(無音訊裝置的 headless 環境)回傳 false — 此時所有播放呼叫都是 no-op。
    virtual bool Initialize() = 0;

    // 關閉並釋放所有聲音與裝置。Idempotent。
    virtual void Shutdown() = 0;

    // 以檔案路徑為 key 載入並解碼 WAV。回傳 buffer handle(失敗為 0)。
    // 相同 key 重複呼叫回傳同一 handle(內容定址快取)。
    virtual uint32_t LoadSound(const char *path) = 0;

    // 在 buffer 上開始播放。loop=true 時循環。回傳 sound 實例 handle(失敗為 0)。
    virtual uint32_t Play(uint32_t buffer, bool loop = false) = 0;

    // 停止並釋放一條 sound 實例。
    virtual void Stop(uint32_t sound) = 0;

    // 暫停 / 恢復一條 sound 實例。
    virtual void Pause(uint32_t sound) = 0;
    virtual void Resume(uint32_t sound) = 0;

    // 設定一條 sound 實例的音量 [0,1]。
    virtual void SetSoundVolume(uint32_t sound, float volume) = 0;

    // 設定整體主音量 [0,1]。
    virtual void SetMasterVolume(float volume) = 0;

    // 釋放一個 buffer(解碼後的 PCM 快取)。
    virtual void ReleaseBuffer(uint32_t buffer) = 0;

    // 目前的 buffer 數量(供測試驗證內容定址去重)。
    virtual uint32_t GetBufferCount() const = 0;

    // 目前的 sound 實例數量。
    virtual uint32_t GetActiveSoundCount() const = 0;
};

#endif // IAUDIOSYSTEM_HPP
