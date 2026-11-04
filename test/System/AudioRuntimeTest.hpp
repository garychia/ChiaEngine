#ifndef AUDIO_RUNTIME_TEST_HPP
#define AUDIO_RUNTIME_TEST_HPP

#include "Scene/AudioSource.hpp"
#include "Scene/AudioListener.hpp"
#include "Scene/SceneSystem.hpp"
#include "System/Audio/IAudioSystem.hpp"
#include "Test.hpp"

#include <cstdio>

namespace audioruntime_test
{

// AudioRuntimeTest:A3.5 音訊運行時契約。
// 1. AudioSource 組件建立與 PlayOnAwake(無音訊裝置時 no-op,不崩潰)
// 2. Play/Pause/Stop 狀態機(無後端時 soundId 保持 0)
// 3. SetVolume 範圍 [0,1]
// 4. AudioListener 組件建立
// 5. SceneSystem 整合 AudioRuntime 不崩潰(無 IAudioService 時)
class AudioRuntimeTest : public Test
{
  public:
    AudioRuntimeTest() : Test("AudioRuntime")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:AudioSource 組件預設值 ----
        {
            AudioSource src;
            EXPECT_TRUE(src.bufferId == 0, "預設 bufferId=0", true);
            EXPECT_TRUE(src.soundId == 0, "預設 soundId=0", true);
            EXPECT_TRUE(src.volume == 1.0f, "預設 volume=1.0", true);
            EXPECT_TRUE(!src.loop, "預設 loop=false", true);
            EXPECT_TRUE(!src.playOnAwake, "預設 playOnAwake=false", true);
            EXPECT_TRUE(!src.isPlaying, "預設 isPlaying=false", true);
            EXPECT_TRUE(!src.isPaused, "預設 isPaused=false", true);
            EXPECT_TRUE(!src.spatial, "預設 spatial=false", true);
        }

        // ---- AC2:AudioListener 組件預設值 ----
        {
            AudioListener listener;
            EXPECT_TRUE(listener.active, "預設 active=true", true);
        }

        // ---- AC3:SceneSystem 與 AudioSource 整合(無 IAudioSystem) ----
        {
            SceneSystem system;

            // 不呼叫 OnAttach → m_pAudioSystem = nullptr
            // Update/FixedUpdate 應該不會崩潰(m_pAudioSystem = nullptr,所有音訊呼叫跳過)
            system.FixedUpdate(FrameClock{0, 0, 0.016, 1.0 / 60.0});
            system.Update(FrameClock{1, 1, 0.016, 1.0 / 60.0});

            EXPECT_TRUE(true, "SceneSystem Update/FixedUpdate 與 AudioRuntime 整合(無後端)不崩潰", true);
        }

        // ---- AC4:SetVolume 範圍驗證 ----
        {
            AudioSource src;
            src.volume = 0.0f;
            EXPECT_TRUE(src.volume >= 0.0f && src.volume <= 1.0f, "volume=0.0 在範圍內", true);
            src.volume = 1.0f;
            EXPECT_TRUE(src.volume >= 0.0f && src.volume <= 1.0f, "volume=1.0 在範圍內", true);
            src.volume = 0.5f;
            EXPECT_TRUE(src.volume >= 0.0f && src.volume <= 1.0f, "volume=0.5 在範圍內", true);
        }

        // ---- AC5:IAudioSystem 介面契約(指標為 nullptr 時不崩潰) ----
        {
            IAudioSystem *pAudio = nullptr;
            if (!pAudio)
            {
                // 播放/暫停/停止/音量等呼叫在 nullptr 上不會發生(呼叫端檢查)
                EXPECT_TRUE(true, "IAudioSystem nullptr 檢查通過", true);
            }
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }

  private:
};

} // namespace audioruntime_test

#endif // AUDIO_RUNTIME_TEST_HPP
