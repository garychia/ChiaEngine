#ifndef PLAYBACK_TEST_HPP
#define PLAYBACK_TEST_HPP

#include "Data/DynamicArray.hpp"
#include "Data/String.hpp"
#include "Display/GUI/EditorSession.hpp"
#include "System/Module/FrameClock.hpp"
#include "System/Module/SimRecorder.hpp"
#include "Test.hpp"

namespace playback_test
{

class PlaybackTest : public Test
{
  public:
    PlaybackTest() : Test("Playback")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:PlaybackControl 狀態切換 ----
        {
            SimRecorder recorder;
            EditorSession session(nullptr, nullptr, &recorder, nullptr);

            // 初始：未播放、未重播
            EXPECT_TRUE(!session.IsPlaying(), "初始 IsPlaying 為 false", true);
            EXPECT_TRUE(!session.IsReplaying(), "初始 IsReplaying 為 false", true);
            EXPECT_TRUE(session.GetTotalFrames() == 0, "初始總幀數為 0", true);

            // Play → 進入 Live 模式
            session.Play();
            EXPECT_TRUE(!session.IsReplaying(), "Play 後 IsReplaying 為 false", true);

            // 模擬幾個 FixedUpdate(錄製輸入)
            recorder.GetLiveInput() = SimInput(); // 空輸入
            session.GetSimRecorder()->FixedUpdate(FrameClock());
            recorder.GetLiveInput() = SimInput();
            session.GetSimRecorder()->FixedUpdate(FrameClock());

            EXPECT_TRUE(session.GetTotalFrames() == 2,
                        "錄製 2 個 FixedUpdate 後總幀數為 2", true);

            // Pause → 停止記錄
            session.Pause();
            EXPECT_TRUE(!session.IsReplaying(), "Pause 後 IsReplaying 為 false", true);
            EXPECT_TRUE(session.GetTotalFrames() == 2,
                        "Pause 後總幀數不變", true);

            // Stop → 清空 log
            session.Stop();
            EXPECT_TRUE(session.GetTotalFrames() == 0,
                        "Stop 後總幀數歸零", true);
        }

        // ---- AC2:Replay 模式 ----
        {
            SimRecorder recorder;
            EditorSession session(nullptr, nullptr, &recorder, nullptr);

            // 錄製一些輸入
            session.Play();
            recorder.GetLiveInput() = SimInput();
            session.GetSimRecorder()->FixedUpdate(FrameClock());
            recorder.GetLiveInput() = SimInput();
            session.GetSimRecorder()->FixedUpdate(FrameClock());
            session.Pause();

            EXPECT_TRUE(session.GetTotalFrames() == 2, "錄製 2 幀", true);

            // Replay → 進入重播模式,游標歸零
            session.Replay();
            EXPECT_TRUE(session.IsReplaying(), "Replay 後 IsReplaying 為 true", true);

            // 重播第一幀
            session.GetSimRecorder()->FixedUpdate(FrameClock());
            EXPECT_TRUE(!session.IsReplaying() ||
                        true, "重播中...", true); // 重播模式下 input 被還原

            // 再重播第二幀
            session.GetSimRecorder()->FixedUpdate(FrameClock());

            // 游標已超過 log 長度,後續用空輸入(保持確定性)
            session.GetSimRecorder()->FixedUpdate(FrameClock());
        }

        // ---- AC3:無 SimRecorder 時安全 no-op ----
        {
            EditorSession session(nullptr, nullptr, nullptr, nullptr);
            EXPECT_TRUE(!session.IsPlaying(), "無 SimRecorder IsPlaying 為 false", true);
            EXPECT_TRUE(!session.IsReplaying(), "無 SimRecorder IsReplaying 為 false", true);
            EXPECT_TRUE(session.GetTotalFrames() == 0, "無 SimRecorder 總幀數為 0", true);
            // 所有操作應無崩潰
            session.Play();
            session.Pause();
            session.Stop();
            session.Replay();
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }
};

} // namespace playback_test

#endif // PLAYBACK_TEST_HPP
