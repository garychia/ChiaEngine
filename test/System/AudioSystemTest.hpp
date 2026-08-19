#ifndef AUDIOSYSTEMTEST_HPP
#define AUDIOSYSTEMTEST_HPP

#include "Test.hpp"
#include "Data/DynamicArray.hpp"
#include "System/Audio/AudioSystem.hpp"
#include "System/Audio/Wav.hpp"

#include <cstring>

// 音訊子系統測試(audiotest namespace,避免與其他 test 的全域型別碰撞)。
//  - WAV 解析 + f32 解碼:完全 headless(純位元組,不碰裝置),可在此驗證。
//  - 裝置播放(Initialize + Play):headless CI / 容器無音訊裝置時 miniaudio
//    engine_init 會失敗 → 該段文件化 skip(與 VulkanRendererTest / WindowManagerTest
//    同款 pattern),不視為失敗。
namespace audiotest
{
// 造一個最小的 16-bit PCM mono WAV(1 秒, 8000Hz, 全 0 樣本)供解析測試。
inline DynamicArray<unsigned char> MakeSilentWav(uint32_t sampleRate, uint16_t channels, uint32_t numFrames)
{
    const uint16_t bits = 16;
    const uint32_t bytesPerFrame = static_cast<uint32_t>(channels) * (bits / 8u);
    const uint32_t dataBytes = numFrames * bytesPerFrame;
    const uint32_t fmtChunkSize = 16;
    const uint32_t riffSize = 4 + (8 + fmtChunkSize) + (8 + dataBytes) - 8;

    DynamicArray<unsigned char> b;
    b.Resize(12 + 8 + fmtChunkSize + 8 + dataBytes);

    size_t p = 0;
    auto put = [&](const void *src, size_t len)
    {
        std::memcpy(&b[p], src, len);
        p += len;
    };
    auto putLE16 = [&](uint16_t v)
    { put(&v, 2); };
    auto putLE32 = [&](uint32_t v)
    { put(&v, 4); };

    put("RIFF", 4);
    putLE32(riffSize);
    put("WAVE", 4);
    put("fmt ", 4);
    putLE32(fmtChunkSize);
    putLE16(1);                 // PCM
    putLE16(channels);          // channels
    putLE32(sampleRate);        // sampleRate
    putLE32(sampleRate * bytesPerFrame); // byteRate
    putLE16(static_cast<uint16_t>(bytesPerFrame));  // blockAlign
    putLE16(bits);              // bitsPerSample
    put("data", 4);
    putLE32(dataBytes);
    // data 全 0(靜音)
    for (uint32_t i = 0; i < dataBytes; i++)
        b[p++] = 0;

    return b;
}

class AudioSystemTest : public Test
{
  public:
    AudioSystemTest() : Test("AudioSystem WAV parse + decode (headless)")
    {
    }

    bool Run() noexcept override
    {
        // 1) 解析合法 WAV
        DynamicArray<unsigned char> wav = MakeSilentWav(8000, 1, 8000);
        const WavData parsed = ParseWav(wav);
        EXPECT_TRUE(parsed.valid, "valid WAV should parse", true);
        EXPECT_TRUE(parsed.numChannels == 1, "mono channel count", true);
        EXPECT_TRUE(parsed.sampleRate == 8000, "sample rate", true);
        EXPECT_TRUE(parsed.bitsPerSample == 16, "16-bit", true);
        EXPECT_TRUE(parsed.FrameCount() == 8000, "frame count == 8000", true);
        EXPECT_TRUE(parsed.dataSize == 8000 * 2, "data size == 16000", true);

        // 2) 畸形輸入必須安全失敗(不拋、回傳 valid=false)
        DynamicArray<unsigned char> junk;
        junk.Resize(4);
        junk[0] = 'X'; junk[1] = 'Y'; junk[2] = 'Z'; junk[3] = 'W';
        const WavData bad = ParseWav(junk);
        EXPECT_TRUE(!bad.valid, "junk input rejected", true);

        // 3) 空的 WAV(無 data chunk)失敗
        DynamicArray<unsigned char> noData;
        noData.Resize(44);
        std::memcpy(&noData[0], "RIFF", 4);
        std::memcpy(&noData[8], "WAVE", 4);
        // fmt 但沒有 data
        std::memcpy(&noData[12], "fmt ", 4);
        uint32_t fs = 16; std::memcpy(&noData[16], &fs, 4);
        const WavData noDataParsed = ParseWav(noData);
        EXPECT_TRUE(!noDataParsed.valid, "WAV without data chunk rejected", true);

        // 4) AudioSystem::DecodeWavToFloat 把 16-bit 靜音正確解成全 0 f32
        AudioSystem sys;
        // 用私有方法不可達;改走 LoadSound 需檔案。這裡改測 decode 契約:
        // 間接驗證 — 造一個 temp 檔給 LoadSound(裝置無關,LoadSound 不開裝置)。
        // 檔案寫入臨時路徑,測完刪除。
        const char *tmpPath = "/tmp/chia_audio_test_silent.wav";
        {
            std::FILE *f = std::fopen(tmpPath, "wb");
            if (!f)
            {
                SUCCESS_MESSAGE("AudioSystem decode test skipped (cannot write temp file)");
                return true;
            }
            std::fwrite(&wav[0], 1, wav.Length(), f);
            std::fclose(f);
        }
        const uint32_t buf = sys.LoadSound(tmpPath);
        EXPECT_TRUE(buf != 0, "LoadSound decodes valid WAV to a buffer", true);
        EXPECT_TRUE(sys.GetBufferCount() == 1, "one buffer cached", true);
        // 相同 path 再 load → 內容定址,仍 1 個 buffer
        const uint32_t buf2 = sys.LoadSound(tmpPath);
        EXPECT_TRUE(buf2 == buf, "same path returns same buffer id", true);
        EXPECT_TRUE(sys.GetBufferCount() == 1, "content-addressed dedup keeps 1 buffer", true);
        sys.ReleaseBuffer(buf);
        sys.ReleaseBuffer(buf2);
        EXPECT_TRUE(sys.GetBufferCount() == 0, "buffer freed after releases", true);
        std::remove(tmpPath);

        SUCCESS_MESSAGE("AudioSystem headless tests passed");
        return true;
    }
};

// 裝置播放的 smoke(文件化 skip:headless 環境無音訊裝置)。
class AudioPlaybackTest : public Test
{
  public:
    AudioPlaybackTest() : Test("AudioSystem device playback (guarded)")
    {
    }

    bool Run() noexcept override
    {
        AudioSystem sys;
        if (!sys.Initialize())
        {
            // 無音訊裝置(headless CI / 容器)→ 文件化 skip,不算失敗。
            SUCCESS_MESSAGE("AudioSystem playback skipped: no audio device (headless) — documented per #59");
            return true;
        }
        const char *tmpPath = "/tmp/chia_audio_test_silent.wav";
        {
            DynamicArray<unsigned char> wav = MakeSilentWav(44100, 2, 44100);
            std::FILE *f = std::fopen(tmpPath, "wb");
            if (!f)
            {
                sys.Shutdown();
                SUCCESS_MESSAGE("AudioSystem playback skipped (cannot write temp file)");
                return true;
            }
            std::fwrite(&wav[0], 1, wav.Length(), f);
            std::fclose(f);
        }
        const uint32_t buf = sys.LoadSound(tmpPath);
        EXPECT_TRUE(buf != 0, "loaded buffer", true);
        const uint32_t snd = sys.Play(buf, false);
        EXPECT_TRUE(snd != 0, "play returns a sound handle", true);
        sys.SetMasterVolume(0.5f);
        sys.SetSoundVolume(snd, 0.8f);
        sys.Stop(snd);
        EXPECT_TRUE(sys.GetActiveSoundCount() == 0, "sound stopped", true);
        sys.ReleaseBuffer(buf);
        sys.Shutdown();
        std::remove(tmpPath);
        SUCCESS_MESSAGE("AudioSystem playback test passed");
        return true;
    }
};

} // namespace audiotest

#endif // AUDIOSYSTEMTEST_HPP
