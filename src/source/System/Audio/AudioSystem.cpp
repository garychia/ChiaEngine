#include "System/Audio/AudioSystem.hpp"
#include "System/Audio/Wav.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include "System/Audio/miniaudio.h"

#include <cstdio>
#include <cstring>

#include <fstream>

AudioSystem::AudioSystem()
    : m_pEngine(nullptr), m_initialized(false), m_buffers(), m_sounds(), m_nextBufferId(1), m_nextSoundId(1)
{
}

AudioSystem::~AudioSystem()
{
    Shutdown();
}

bool AudioSystem::Initialize()
{
    if (m_initialized)
        return true;

    ma_engine_config engineConfig = ma_engine_config_init();
    ma_engine *pEngine = new (std::nothrow) ma_engine;
    if (!pEngine)
        return false;

    const ma_result res = ma_engine_init(&engineConfig, pEngine);
    if (res != MA_SUCCESS)
    {
        delete pEngine;
        return false;
    }

    m_pEngine = pEngine;
    m_initialized = true;
    return true;
}

void AudioSystem::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized)
        return;

    // 停掉並釋放所有 sound(含其 ma_audio_buffer)
    while (m_sounds.First() != m_sounds.Last())
    {
        AudioSound *s = m_sounds.First()->Value();
        if (s)
        {
            if (s->pMaSound)
                ma_sound_uninit(static_cast<ma_sound *>(s->pMaSound));
            if (s->pMaBuffer)
            {
                ma_audio_buffer_uninit(static_cast<ma_audio_buffer *>(s->pMaBuffer));
                delete static_cast<ma_audio_buffer *>(s->pMaBuffer);
            }
            delete s;
        }
        m_sounds.Remove(m_sounds.First()->Key());
    }

    // 釋放所有 buffer 的 PCM
    while (m_buffers.First() != m_buffers.Last())
    {
        AudioBuffer *b = m_buffers.First()->Value();
        if (b)
        {
            delete[] b->pPcm;
            delete b;
        }
        m_buffers.Remove(m_buffers.First()->Key());
    }

    if (m_pEngine)
    {
        ma_engine_uninit(m_pEngine);
        delete m_pEngine;
        m_pEngine = nullptr;
    }
    m_initialized = false;
}

uint32_t AudioSystem::LoadSound(const char *path)
{
    if (!path)
        return 0;

    std::lock_guard<std::mutex> lock(m_mutex);

    // 內容定址:相同 path 直接回傳既有 buffer(並 +1 引用)
    HashTable<uint32_t, AudioBuffer *>::Iterator it = m_buffers.First();
    while (it != m_buffers.Last())
    {
        if (std::strcmp(it->Value()->path, path) == 0)
        {
            it->Value()->refCount++;
            return it->Value()->id;
        }
        ++it;
    }

    // 首版走同步讀檔解碼(minimal first pass,issue #59 明定 out-of-scope
    // 為非同步串流)。未來可改接 AssetManager::LoadAsync + OnAssetLoaded(見 PR 說明)。
    DynamicArray<unsigned char> raw;
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            return 0;
        file.seekg(0, std::ios::end);
        const std::streamoff size = file.tellg();
        file.seekg(0, std::ios::beg);
        const size_t n = size > 0 ? static_cast<size_t>(size) : 0;
        raw.Resize(n);
        if (n > 0)
            file.read(reinterpret_cast<char *>(&raw[0]), static_cast<std::streamsize>(n));
    }
    if (raw.Length() == 0)
        return 0;

    AudioBuffer *b = new (std::nothrow) AudioBuffer;
    if (!b)
        return 0;
    std::memset(b, 0, sizeof(AudioBuffer));
    b->id = m_nextBufferId++;
    std::strncpy(b->path, path, sizeof(b->path) - 1);
    b->refCount = 1;
    b->loaded = false;

    if (!DecodeWavToFloat(&raw[0], raw.Length(), *b))
    {
        delete b;
        return 0;
    }
    b->loaded = true;
    m_buffers.Insert(b->id, b);
    return b->id;
}

bool AudioSystem::DecodeWavToFloat(const unsigned char *bytes, size_t size, AudioBuffer &out)
{
    // 同步讀檔保證 size>0 時 bytes 有效;size==0 直接失敗
    if (size == 0)
        return false;

    // ParseWav 吃 DynamicArray,用一個 view 包住 raw bytes(不複製)
    DynamicArray<unsigned char> view;
    view.Resize(size);
    std::memcpy(&view[0], bytes, size);

    const WavData wav = ParseWav(view);
    if (!wav.valid || wav.dataSize == 0)
        return false;
    if (wav.bitsPerSample != 16 && wav.bitsPerSample != 8)
        return false; // minimal first pass:只支援 8/16-bit PCM

    const uint32_t frames = wav.FrameCount();
    if (frames == 0)
        return false;

    // 解碼成 f32 交錯(ma 預設格式):每 frame = channels 個 f32
    float *pcm = new (std::nothrow) float[static_cast<size_t>(frames) * wav.numChannels];
    if (!pcm)
        return false;

    const unsigned char *src = bytes + wav.dataOffset;
    if (wav.bitsPerSample == 16)
    {
        for (uint32_t i = 0; i < frames * wav.numChannels; i++)
        {
            int16_t s;
            std::memcpy(&s, src + i * 2, 2);
            pcm[i] = static_cast<float>(s) / 32768.0f;
        }
    }
    else // 8-bit:無符號,中心 128
    {
        for (uint32_t i = 0; i < frames * wav.numChannels; i++)
            pcm[i] = (static_cast<float>(src[i]) - 128.0f) / 128.0f;
    }

    out.pPcm = pcm;
    out.pcmFrameCount = frames;
    out.channels = wav.numChannels;
    out.sampleRate = wav.sampleRate;
    return true;
}

uint32_t AudioSystem::Play(uint32_t buffer, bool loop)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_pEngine)
        return 0;

    HashTable<uint32_t, AudioBuffer *>::Iterator bit = m_buffers.Find(buffer);
    if (bit == m_buffers.Last() || !bit->Value()->loaded)
        return 0;

    AudioBuffer *b = bit->Value();

    // in-memory audio buffer datasource(miniaudio 不接管,我們在 Stop 裡手動 uninit)
    // ma_audio_buffer 必須是「已配置的結構」— ma_audio_buffer_init 寫入它,
    // 所以這裡 new 一顆,生命周期交給 AudioSound(Stop 時 uninit + delete)。
    ma_audio_buffer *pMaBuf = new (std::nothrow) ma_audio_buffer;
    if (!pMaBuf)
        return 0;
    ma_audio_buffer_config bufCfg =
        ma_audio_buffer_config_init(ma_format_f32, b->channels, b->pcmFrameCount, b->pPcm, nullptr);
    if (ma_audio_buffer_init(&bufCfg, pMaBuf) != MA_SUCCESS)
    {
        delete pMaBuf;
        return 0;
    }

    ma_sound *pSound = new (std::nothrow) ma_sound;
    if (!pSound)
    {
        ma_audio_buffer_uninit(pMaBuf);
        delete pMaBuf;
        return 0;
    }
    const ma_result res = ma_sound_init_from_data_source(m_pEngine, pMaBuf, 0, nullptr, pSound);
    if (res != MA_SUCCESS)
    {
        ma_sound_uninit(pSound);
        ma_audio_buffer_uninit(pMaBuf);
        delete pSound;
        delete pMaBuf;
        return 0;
    }
    ma_sound_set_looping(pSound, loop ? MA_TRUE : MA_FALSE);

    AudioSound *s = new (std::nothrow) AudioSound;
    if (!s)
    {
        ma_sound_uninit(pSound);
        ma_audio_buffer_uninit(pMaBuf);
        delete pSound;
        delete pMaBuf;
        return 0;
    }
    std::memset(s, 0, sizeof(AudioSound));
    s->id = m_nextSoundId++;
    s->bufferId = buffer;
    s->pEngine = m_pEngine;
    s->pMaSound = pSound;
    s->pMaBuffer = pMaBuf;
    s->inUse = true;

    b->refCount++; // buffer 被一條 sound 引用
    ma_sound_start(pSound);

    m_sounds.Insert(s->id, s);
    return s->id;
}

void AudioSystem::Stop(uint32_t sound)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    HashTable<uint32_t, AudioSound *>::Iterator it = m_sounds.Find(sound);
    if (it == m_sounds.Last())
        return;
    AudioSound *s = it->Value();
    if (s)
    {
        if (s->pMaSound)
            ma_sound_uninit(static_cast<ma_sound *>(s->pMaSound));
        if (s->pMaBuffer)
        {
            ma_audio_buffer_uninit(static_cast<ma_audio_buffer *>(s->pMaBuffer));
            delete static_cast<ma_audio_buffer *>(s->pMaBuffer);
        }
        // 歸還 buffer 引用
        HashTable<uint32_t, AudioBuffer *>::Iterator bit = m_buffers.Find(s->bufferId);
        if (bit != m_buffers.Last() && bit->Value()->refCount > 0)
            bit->Value()->refCount--;
        delete s;
    }
    m_sounds.Remove(sound);
}

void AudioSystem::Pause(uint32_t sound)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    HashTable<uint32_t, AudioSound *>::Iterator it = m_sounds.Find(sound);
    if (it == m_sounds.Last() || !it->Value()->pMaSound)
        return;
    ma_sound_stop(static_cast<ma_sound *>(it->Value()->pMaSound));
}

void AudioSystem::Resume(uint32_t sound)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    HashTable<uint32_t, AudioSound *>::Iterator it = m_sounds.Find(sound);
    if (it == m_sounds.Last() || !it->Value()->pMaSound)
        return;
    ma_sound_start(static_cast<ma_sound *>(it->Value()->pMaSound));
}

void AudioSystem::SetSoundVolume(uint32_t sound, float volume)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    HashTable<uint32_t, AudioSound *>::Iterator it = m_sounds.Find(sound);
    if (it == m_sounds.Last() || !it->Value()->pMaSound)
        return;
    ma_sound_set_volume(static_cast<ma_sound *>(it->Value()->pMaSound), volume);
}

void AudioSystem::SetMasterVolume(float volume)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_pEngine)
        return;
    ma_engine_set_volume(m_pEngine, volume);
}

void AudioSystem::ReleaseBuffer(uint32_t buffer)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    HashTable<uint32_t, AudioBuffer *>::Iterator it = m_buffers.Find(buffer);
    if (it == m_buffers.Last() || !it->Value())
        return;
    AudioBuffer *b = it->Value();
    if (b->refCount > 0)
        b->refCount--;
    if (b->refCount == 0)
    {
        delete[] b->pPcm;
        delete b;
        m_buffers.Remove(buffer);
    }
}

uint32_t AudioSystem::GetBufferCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    uint32_t c = 0;
    HashTable<uint32_t, AudioBuffer *>::Iterator it = m_buffers.First();
    while (it != m_buffers.Last())
    {
        c++;
        ++it;
    }
    return c;
}

uint32_t AudioSystem::GetActiveSoundCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    uint32_t c = 0;
    HashTable<uint32_t, AudioSound *>::Iterator it = m_sounds.First();
    while (it != m_sounds.Last())
    {
        if (it->Value() && it->Value()->inUse)
            c++;
        ++it;
    }
    return c;
}
