#ifndef WAV_HPP
#define WAV_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "Data/DynamicArray.hpp"

// 最小 WAV (PCM) 解碼器 — 純位元組解析,不依賴任何音訊裝置,
// 因此可以在 TestMain (headless) 中完整驗證。
//
// 支援:標準 RIFF/WAVE,fmt chunk 為 PCM (audioFormat == 1)。
// 只解析到「資料區」的起點與長度,真正的重採樣/混音交給 miniaudio。
// 超出本 minimal first pass 的壓縮格式(WAV 內的 ADPCM/MP3 等)視為失敗。
struct WavData
{
    uint16_t numChannels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    uint32_t dataOffset = 0; // 相對於原始 bytes 的 PCM 起點
    uint32_t dataSize = 0;   // PCM 位元組數
    bool valid = false;

    // 幀數 = dataSize / (numChannels * bitsPerSample/8)
    uint32_t FrameCount() const
    {
        if (!valid || bitsPerSample == 0 || numChannels == 0)
            return 0;
        const uint32_t bytesPerFrame = static_cast<uint32_t>(numChannels) * (bitsPerSample / 8u);
        if (bytesPerFrame == 0)
            return 0;
        return dataSize / bytesPerFrame;
    }

    uint32_t BytesPerFrame() const
    {
        if (bitsPerSample == 0 || numChannels == 0)
            return 0;
        return static_cast<uint32_t>(numChannels) * (bitsPerSample / 8u);
    }
};

// 從原始 bytes 解析 WAV header。失敗時回傳 valid=false 的 WavData。
// 設計成 noexcept / 不拋 — 解析器必須對畸形輸入安全(見 AssetFormatValidation 契約)。
inline WavData ParseWav(const DynamicArray<unsigned char> &bytes)
{
    WavData out;
    const size_t n = bytes.Length();
    // RIFF<4> WAVE + 至少 fmt(16) + data header(8) 的最小長度
    if (n < 12 + 24)
        return out;

    // RIFF 標記
    if (std::memcmp(&bytes[0], "RIFF", 4) != 0)
        return out;
    if (std::memcmp(&bytes[8], "WAVE", 4) != 0)
        return out;

    // 線性掃描 chunk:每個 = 4-byte id + 4-byte size(little-endian) + payload
    size_t pos = 12;
    bool haveFmt = false;
    while (pos + 8 <= n)
    {
        char id[4];
        std::memcpy(id, &bytes[pos], 4);
        uint32_t chunkSize = 0;
        std::memcpy(&chunkSize, &bytes[pos + 4], 4);
        const size_t payload = pos + 8;

        if (std::memcmp(id, "fmt ", 4) == 0)
        {
            // fmt payload: audioFormat(2) channels(2) sampleRate(4)
            //             byteRate(4) blockAlign(2) bitsPerSample(2) [+ext]
            if (payload + 16 > n)
                return out;
            uint16_t audioFormat = 0;
            std::memcpy(&audioFormat, &bytes[payload + 0], 2);
            std::memcpy(&out.numChannels, &bytes[payload + 2], 2);
            std::memcpy(&out.sampleRate, &bytes[payload + 4], 4);
            std::memcpy(&out.bitsPerSample, &bytes[payload + 14], 2);
            if (audioFormat != 1) // 只支援 PCM
                return out;
            if (out.numChannels == 0 || out.bitsPerSample == 0)
                return out;
            haveFmt = true;
        }
        else if (std::memcmp(id, "data", 4) == 0)
        {
            out.dataOffset = static_cast<uint32_t>(payload);
            out.dataSize = chunkSize;
            // 邊界防護:data 不能超出檔案
            if (out.dataOffset + out.dataSize > n)
                out.dataSize = static_cast<uint32_t>(n - out.dataOffset);
            // data chunk 不一定在 fmt 之後;找到就記下,但 valid 需等 fmt 也解析完
            if (haveFmt)
            {
                out.valid = true;
                return out;
            }
        }

        // chunk 對齊到偶數位元組
        size_t next = payload + chunkSize;
        if (chunkSize & 1u)
            next += 1;
        if (next <= pos)
            break;
        pos = next;
    }

    // 走到這裡表示找到 fmt 但沒找到 data chunk
    out.valid = false;
    return out;
}

#endif // WAV_HPP
