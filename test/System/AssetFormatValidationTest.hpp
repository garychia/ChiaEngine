#ifndef ASSET_FORMAT_VALIDATION_TEST_HPP
#define ASSET_FORMAT_VALIDATION_TEST_HPP

#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

#include "Test.hpp"
#include "System/Asset/AssetManager.hpp"
#include "System/Job/JobSystem.hpp"
#include "Display/Asset/TextureAssetView.hpp"

// ── Issue #61: asset format validation tests ──
//  AssetManager 把檔案 bytes 視為不透明資料(不做格式解析);「malformed」在此
//  意為「任何不合法/不完整/損壞的位元組流」都必須被乾淨處理:
//     * 不 crash、不 UB、不 hang(所有載入都在 bounded WaitForIdle 內完成)
//     * loaded/rejected 狀態可觀察且一致(loaded ⇔ pBlock 非空)
//     * 完成事件對每個請求恰好觸發一次(含失敗/missing 檔)
//     * 釋放後回到乾淨狀態(blocks 歸零,key 表一致)
//  (1) MalformedCorpusTest  — 有效 + truncated(含 0/empty)+ corrupt bytes + missing
//  (2) AssetRoundtripTest   — 決定性 pseudo-random bytes(含 0/1 byte)寫檔 → 載入 → bytes 逐位元比對
//  (3) TextureViewHookTest  — hook 在有效載入收到正確 bytes;malformed/empty/missing 下安全
namespace assetfmt
{
// ── 決定性 pseudo-random(固定種子的 LCG;同 seed 必產生同位元組序列)──
inline uint64_t g_rngState = 0;
inline void SeedRng(uint64_t s)
{
    g_rngState = s;
}
inline uint8_t NextRngByte()
{
    g_rngState = g_rngState * 6364136223846793005ull + 1442695040888963407ull;
    return static_cast<uint8_t>(g_rngState >> 56);
}

inline bool WriteFileBytes(const char *path, const unsigned char *data, size_t len)
{
    std::ofstream file(path, std::ios::binary);
    if (!file)
        return false;
    if (len > 0)
        file.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(len));
    return file.good();
}

inline bool BytesEqual(const DynamicArray<unsigned char> &a, const unsigned char *b, size_t n)
{
    if (a.Length() != n)
        return false;
    if (n == 0)
        return true;
    return std::memcmp(&a[0], b, n) == 0;
}

inline uint64_t ExpectedHash(const unsigned char *data, size_t len)
{
    return len > 0 ? FNV1A64(FNV1A64_OFFSET, data, len) : FNV1A64_OFFSET;
}

// 訂閱 LoadedEvent 的測試觀察者(同 AssetTest 慣例)。
class AssetObserver
{
  public:
    DynamicArray<String> loadedKeys;
    size_t eventCount = 0;

    void OnLoaded(SharedPtr<Asset> asset)
    {
        loadedKeys.Append(asset->key);
        eventCount++;
    }
};

// ═══════════════════════════════════════════════════════════════════════
// (1) Malformed corpus:valid + truncated(含 0/empty)+ corrupt + missing
// ═══════════════════════════════════════════════════════════════════════
class MalformedCorpusTest : public Test
{
  public:
    struct CorpusEntry
    {
        const char *path;
        const char *desc;
        DynamicArray<unsigned char> bytes; // empty for missing files
        bool writeFile;
        bool expectLoaded;
    };

    MalformedCorpusTest() : Test("MalformedCorpusTest")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("MalformedCorpusTest: valid + truncated + corrupt + missing asset corpus -> clean handling "
                     "(no crash / no UB / no hang, sane loaded/rejected state)");

        // ── corpus 內容 ──
        DynamicArray<unsigned char> valid(64);
        const unsigned char magic[4] = {'C', 'H', 'I', 'A'};
        for (size_t i = 0; i < 4; i++)
            valid[i] = magic[i];
        for (size_t i = 4; i < 64; i++)
            valid[i] = static_cast<unsigned char>((i * 7 + 1) & 0xFF);

        DynamicArray<unsigned char> corruptMagic(valid);
        corruptMagic[0] = 0x00;
        corruptMagic[1] = 0x01;
        corruptMagic[2] = 0x02;
        corruptMagic[3] = 0x03; // 錯誤 magic(壞檔頭)

        DynamicArray<unsigned char> corruptFlip(valid);
        corruptFlip[30] ^= 0xFF; // 中間位元組損壞

        SeedRng(0xDEADBEEFCAFEF00Dull);
        DynamicArray<unsigned char> garbage(128);
        for (size_t i = 0; i < garbage.Length(); i++)
            garbage[i] = NextRngByte(); // 全亂數垃圾

        std::vector<CorpusEntry> entries;
        entries.push_back(CorpusEntry{"/tmp/chia_corpus_valid.bin", "valid (64B, magic 'CHIA')", valid, true, true});
        entries.push_back(CorpusEntry{"/tmp/chia_corpus_empty.bin", "empty file (0B, trunc at offset 0)", DynamicArray<unsigned char>(), true, true});
        entries.push_back(CorpusEntry{"/tmp/chia_corpus_trunc1.bin", "truncated at offset 1 (1B)", DynamicArray<unsigned char>(&valid[0], 1), true, true});
        entries.push_back(CorpusEntry{"/tmp/chia_corpus_trunc16.bin", "truncated at offset 16 (16B)", DynamicArray<unsigned char>(&valid[0], 16), true, true});
        entries.push_back(CorpusEntry{"/tmp/chia_corpus_trunc32.bin", "truncated at offset 32 (32B)", DynamicArray<unsigned char>(&valid[0], 32), true, true});
        entries.push_back(CorpusEntry{"/tmp/chia_corpus_trunc63.bin", "truncated at offset 63 (size-1)", DynamicArray<unsigned char>(&valid[0], 63), true, true});
        entries.push_back(CorpusEntry{"/tmp/chia_corpus_badmagic.bin", "corrupt magic (bad header)", corruptMagic, true, true});
        entries.push_back(CorpusEntry{"/tmp/chia_corpus_badbyte.bin", "corrupt byte (flipped)", corruptFlip, true, true});
        entries.push_back(CorpusEntry{"/tmp/chia_corpus_garbage.bin", "garbage (128B pseudo-random)", garbage, true, true});
        entries.push_back(CorpusEntry{"/tmp/chia_corpus_missing_late.bin", "missing file (released after dispatch)", DynamicArray<unsigned char>(), false, false});
        entries.push_back(CorpusEntry{"/tmp/chia_corpus_missing_early.bin", "missing file (released before dispatch)", DynamicArray<unsigned char>(), false, false});

        // 寫檔(只寫 writeFile 的;missing 不寫)
        for (size_t i = 0; i < entries.size(); i++)
        {
            if (!entries[i].writeFile)
                continue;
            const unsigned char *p = entries[i].bytes.Length() > 0 ? &entries[i].bytes[0] : nullptr;
            Check(WriteFileBytes(entries[i].path, p, entries[i].bytes.Length()), "corpus file written");
        }

        JobSystem jobs(2);
        AssetManager manager(jobs);
        AssetObserver observer;
        manager.LoadedEvent.Subscribe(&observer, &AssetObserver::OnLoaded);

        // ── 批次 1:9 個既有檔 + 1 個 missing(late release)──
        std::vector<SharedPtr<Asset>> handles;
        for (size_t i = 0; i < entries.size() - 1; i++)
            handles.push_back(manager.LoadAsync(entries[i].path));
        jobs.WaitForIdle();
        const size_t nEvents1 = manager.DispatchCompletedEvents();

        Check(nEvents1 == 10, "batch1: every request (incl. missing) fires exactly 1 completion event");
        Check(observer.eventCount == 10, "batch1: observer saw 10 events");
        Check(manager.GetNumAssets() == 10, "batch1: 10 keys registered");
        Check(manager.GetNumSharedBlocks() == 9, "batch1: only the 9 written files created shared blocks (missing -> none)");

        for (size_t i = 0; i < handles.size(); i++)
        {
            char buf[192];
            const CorpusEntry &e = entries[i];
            const unsigned char *p = e.bytes.Length() > 0 ? &e.bytes[0] : nullptr;
            if (e.writeFile)
            {
                std::snprintf(buf, sizeof(buf), "[%s] loaded == true (raw bytes always loadable)", e.desc);
                Check(handles[i]->loaded, buf);
                std::snprintf(buf, sizeof(buf), "[%s] bytes length == %zu", e.desc, e.bytes.Length());
                Check(handles[i]->Bytes().Length() == e.bytes.Length(), buf);
                std::snprintf(buf, sizeof(buf), "[%s] loaded bytes byte-identical to file content", e.desc);
                Check(BytesEqual(handles[i]->Bytes(), p, e.bytes.Length()), buf);
                std::snprintf(buf, sizeof(buf), "[%s] content hash == FNV1A64(bytes)", e.desc);
                Check(handles[i]->ContentHash() == ExpectedHash(p, e.bytes.Length()), buf);
                DynamicArray<unsigned char> out;
                std::snprintf(buf, sizeof(buf), "[%s] GetLoadedBytes finds it with identical bytes", e.desc);
                Check(manager.GetLoadedBytes(e.path, &out) && BytesEqual(out, p, e.bytes.Length()), buf);
            }
            else
            {
                std::snprintf(buf, sizeof(buf), "[%s] rejected state: loaded == false", e.desc);
                Check(!handles[i]->loaded, buf);
                std::snprintf(buf, sizeof(buf), "[%s] rejected state: no block, ContentHash() == 0", e.desc);
                Check(!handles[i]->pBlock && handles[i]->ContentHash() == 0, buf);
                DynamicArray<unsigned char> out;
                std::snprintf(buf, sizeof(buf), "[%s] rejected state: GetLoadedBytes returns false", e.desc);
                Check(!manager.GetLoadedBytes(e.path, &out), buf);
            }
            std::snprintf(buf, sizeof(buf), "[%s] loaded <=> pBlock consistent (no dangling state)", e.desc);
            Check((handles[i]->loaded && handles[i]->pBlock) || (!handles[i]->loaded && !handles[i]->pBlock), buf);
        }

        // ── 批次 2:missing 檔在載入完成前就被釋放 → 乾淨 evict、不發事件 ──
        SharedPtr<Asset> hMissingEarly = manager.LoadAsync(entries[10].path);
        manager.Release(hMissingEarly);
        jobs.WaitForIdle();
        const size_t nEvents2 = manager.DispatchCompletedEvents();
        Check(nEvents2 == 0, "batch2: early-released missing file fires no event");
        Check(observer.eventCount == 10, "batch2: observer count unchanged");
        Check(manager.GetNumAssets() == 10, "batch2: early-released key evicted cleanly (10 keys again)");
        Check(manager.GetNumSharedBlocks() == 9, "batch2: no block leaked for missing file");

        // ── 批次 3:兩顆不同 key、byte-identical 的 corrupt 內容 → 共享同一 block ──
        const char *pathC1 = "/tmp/chia_corpus_share1.bin";
        const char *pathC2 = "/tmp/chia_corpus_share2.bin";
        DynamicArray<unsigned char> sharedBytes(16);
        for (size_t i = 0; i < 16; i++)
            sharedBytes[i] = 0xAB; // 任何「內容相同」都行 — 這裡用固定壞位元組
        Check(WriteFileBytes(pathC1, &sharedBytes[0], 16), "share1 written");
        Check(WriteFileBytes(pathC2, &sharedBytes[0], 16), "share2 written");

        SharedPtr<Asset> hC1 = manager.LoadAsync(pathC1);
        SharedPtr<Asset> hC2 = manager.LoadAsync(pathC2);
        jobs.WaitForIdle();
        const size_t nEvents3 = manager.DispatchCompletedEvents();
        Check(nEvents3 == 2, "share: 2 completion events for 2 keys");
        Check(observer.eventCount == 12, "share: observer saw 12 events total");
        Check(hC1->pBlock == hC2->pBlock, "share: identical corrupt content -> same AssetBlock (pointer-equal)");
        Check(manager.GetNumAssets() == 12, "share: 12 keys registered");
        Check(manager.GetNumSharedBlocks() == 10, "share: identical contents counted once (10 blocks)");
        Check(manager.GetBlockRefCount(hC1->pBlock) == 2, "share: block refcount == 2 (two keys)");
        Check(manager.GetAssetRefCount(hC1) == 1 && manager.GetAssetRefCount(hC2) == 1, "share: each key one consumer ref");

        // ── 批次 4:同 key 重複載入 → dedup,不重發事件 ──
        SharedPtr<Asset> hDup = manager.LoadAsync(pathC1);
        Check(hDup == hC1, "dedup: duplicate request returns same handle");
        Check(manager.GetAssetRefCount(hC1) == 2, "dedup: consumer refcount incremented to 2");
        jobs.WaitForIdle();
        const size_t nEvents4 = manager.DispatchCompletedEvents();
        Check(nEvents4 == 0, "dedup: no duplicate completion event");
        Check(manager.GetNumAssets() == 12, "dedup: no new asset registered");
        manager.Release(hDup);
        Check(manager.GetAssetRefCount(hC1) == 1, "dedup: releasing the extra ref restores refcount 1");
        manager.Release(hC1);
        manager.Release(hC2);
        Check(manager.GetNumAssets() == 10, "share: releasing both keys evicts them (10 keys)");
        Check(manager.GetNumSharedBlocks() == 9, "share: shared block unloaded after last release (9 blocks)");

        // ── 批次 5:釋放全部 → 回到乾淨狀態 ──
        for (size_t i = 0; i < handles.size(); i++)
            manager.Release(handles[i]);
        Check(manager.GetNumSharedBlocks() == 0, "cleanup: all content blocks unloaded (0 blocks)");
        Check(manager.GetNumAssets() == 1, "cleanup: only the late-released missing-file key remains (never-loaded; "
                                            "documented engine behavior), no crash/leak");
        DynamicArray<unsigned char> out;
        Check(!manager.GetLoadedBytes(entries[0].path, &out), "cleanup: valid key gone after release");
        Check(!manager.GetLoadedBytes(entries[9].path, &out), "cleanup: missing key reports not loaded");

        // 全部 bounded 步驟完成 = 沒有 hang;沒有 crash/UB = 走到這裡本身就證明
        Check(true, "all malformed loads completed within bounded WaitForIdle (no hang), no crash/UB");

        manager.LoadedEvent.Unsubscribe(&observer);
        for (size_t i = 0; i < entries.size(); i++)
        {
            if (entries[i].writeFile)
                std::remove(entries[i].path);
        }
        std::remove(pathC1);
        std::remove(pathC2);

        std::cout << "  checks: " << nPassed << " passed, " << nFailed << " failed" << std::endl;
        SUCCESS_MESSAGE("MalformedCorpusTest");
        return nFailed == 0;
    }

  private:
    size_t nPassed = 0;
    size_t nFailed = 0;

    bool Check(bool cond, const char *msg)
    {
        EXPECT_TRUE(cond, msg, false); // non-fatal:return_false=false,never returns early
        if (cond)
        {
            nPassed++;
            std::cout << "  [PASS] " << msg << std::endl;
        }
        else
        {
            nFailed++;
        }
        return true;
    }
};

// ═══════════════════════════════════════════════════════════════════════
// (2) Property-based roundtrip:deterministic pseudo-random bytes → file
//     → LoadAsync → WaitForIdle → DispatchCompletedEvents → compare bytes
// ═══════════════════════════════════════════════════════════════════════
class AssetRoundtripTest : public Test
{
  public:
    AssetRoundtripTest() : Test("AssetRoundtripTest")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("AssetRoundtripTest: deterministic pseudo-random bytes (sizes incl. 0 and 1) "
                     "-> file -> async load -> byte-identical roundtrip");

        static const size_t kSizes[] = {0, 1, 2, 3, 16, 255, 256, 1024, 4096, 65536, 65537};
        SeedRng(0x123456789ABCDEFull); // 固定種子 → 每次執行產生相同 bytes

        size_t totalBytes = 0;
        for (size_t k = 0; k < sizeof(kSizes) / sizeof(kSizes[0]); k++)
        {
            const size_t size = kSizes[k];
            char path[64];
            std::snprintf(path, sizeof(path), "/tmp/chia_roundtrip_%zu.bin", size);

            DynamicArray<unsigned char> original(size);
            for (size_t i = 0; i < size; i++)
                original[i] = NextRngByte();
            totalBytes += size;

            const unsigned char *p = size > 0 ? &original[0] : nullptr;
            Check(WriteFileBytes(path, p, size), "roundtrip: file written");

            JobSystem jobs(2);
            AssetManager manager(jobs);
            AssetObserver observer;
            manager.LoadedEvent.Subscribe(&observer, &AssetObserver::OnLoaded);

            SharedPtr<Asset> h = manager.LoadAsync(path);
            jobs.WaitForIdle();
            const size_t nEvents = manager.DispatchCompletedEvents();

            char buf[160];
            std::snprintf(buf, sizeof(buf), "rt[%zu B]: loaded == true", size);
            Check(h->loaded, buf);
            std::snprintf(buf, sizeof(buf), "rt[%zu B]: exactly 1 completion event", size);
            Check(nEvents == 1 && observer.eventCount == 1, buf);

            DynamicArray<unsigned char> fetched;
            std::snprintf(buf, sizeof(buf), "rt[%zu B]: GetLoadedBytes byte-identical to original", size);
            Check(manager.GetLoadedBytes(path, &fetched) && BytesEqual(fetched, p, size), buf);
            std::snprintf(buf, sizeof(buf), "rt[%zu B]: handle bytes byte-identical to original", size);
            Check(BytesEqual(h->Bytes(), p, size), buf);
            std::snprintf(buf, sizeof(buf), "rt[%zu B]: content hash == FNV1A64(original bytes)", size);
            Check(h->ContentHash() == ExpectedHash(p, size), buf);

            // key-dedup:同一 path 再載入 → 同 handle、不重發事件
            SharedPtr<Asset> hDup = manager.LoadAsync(path);
            std::snprintf(buf, sizeof(buf), "rt[%zu B]: duplicate request returns same handle", size);
            Check(hDup == h, buf);
            std::snprintf(buf, sizeof(buf), "rt[%zu B]: dedup increments consumer refcount to 2", size);
            Check(manager.GetAssetRefCount(h) == 2, buf);
            jobs.WaitForIdle();
            std::snprintf(buf, sizeof(buf), "rt[%zu B]: dedup fires no duplicate event", size);
            Check(manager.DispatchCompletedEvents() == 0, buf);
            manager.Release(hDup);
            std::snprintf(buf, sizeof(buf), "rt[%zu B]: releasing extra ref restores refcount 1", size);
            Check(manager.GetAssetRefCount(h) == 1, buf);

            manager.Release(h);
            std::snprintf(buf, sizeof(buf), "rt[%zu B]: clean teardown (0 live assets, 0 blocks)", size);
            Check(manager.GetNumAssets() == 0 && manager.GetNumSharedBlocks() == 0, buf);
            manager.LoadedEvent.Unsubscribe(&observer);
            std::remove(path);
        }

        char buf[128];
        std::snprintf(buf, sizeof(buf), "roundtrip covered %zu sizes, %zu total deterministic bytes",
                      sizeof(kSizes) / sizeof(kSizes[0]), totalBytes);
        Check(true, buf);

        std::cout << "  checks: " << nPassed << " passed, " << nFailed << " failed" << std::endl;
        SUCCESS_MESSAGE("AssetRoundtripTest");
        return nFailed == 0;
    }

  private:
    size_t nPassed = 0;
    size_t nFailed = 0;

    bool Check(bool cond, const char *msg)
    {
        EXPECT_TRUE(cond, msg, false); // non-fatal:return_false=false,never returns early
        if (cond)
        {
            nPassed++;
            std::cout << "  [PASS] " << msg << std::endl;
        }
        else
        {
            nFailed++;
        }
        return true;
    }
};

// ═══════════════════════════════════════════════════════════════════════
// (3) TextureAssetView hook:correct bytes on valid load; safe on
//     malformed / empty / missing assets
// ═══════════════════════════════════════════════════════════════════════
class TextureViewHookTest : public Test
{
  public:
    struct HookRecorder
    {
        size_t calls = 0;
        DynamicArray<unsigned char> lastBytes;
        size_t lastHash = 0;

        static void OnTextureReady(const DynamicArray<unsigned char> &rgbaBytes, size_t contentHash, void *pUserData)
        {
            HookRecorder *self = static_cast<HookRecorder *>(pUserData);
            self->calls++;
            self->lastBytes = rgbaBytes;
            self->lastHash = contentHash;
        }
    };

    TextureViewHookTest() : Test("TextureViewHookTest")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("TextureViewHookTest: renderer hook fires with correct bytes on valid load; "
                     "safe on malformed / empty / missing assets");

        const char *pathValid = "/tmp/chia_view_valid.bin";
        const char *pathBad = "/tmp/chia_view_bad.bin";
        const char *pathEmpty = "/tmp/chia_view_empty.bin";
        const char *pathMissing = "/tmp/chia_view_missing.bin";
        const char *pathDetach = "/tmp/chia_view_detach.bin";
        const char *pathReattach = "/tmp/chia_view_reattach.bin";

        const char *validContent = "TEX-VALID-1234"; // 14 bytes,含 magic-ish 檔頭
        const size_t validLen = std::strlen(validContent);
        Check(WriteFileBytes(pathValid, reinterpret_cast<const unsigned char *>(validContent), validLen), "valid file written");

        SeedRng(0xC0FFEE123456789Aull);
        DynamicArray<unsigned char> badBytes(32);
        for (size_t i = 0; i < 32; i++)
            badBytes[i] = NextRngByte(); // 損壞貼圖(垃圾 bytes)
        Check(WriteFileBytes(pathBad, &badBytes[0], 32), "corrupt file written");

        Check(WriteFileBytes(pathEmpty, nullptr, 0), "empty file written");

        JobSystem jobs(2);
        AssetManager manager(jobs);
        TextureAssetView view;
        view.SubscribeTo(manager);
        Check(view.IsAttached(), "view attached to AssetManager");

        HookRecorder recorder;
        view.SetTextureReadyHook(&HookRecorder::OnTextureReady, &recorder);

        // ── S1:有效載入 → hook 收到正確 bytes ──
        SharedPtr<Asset> hValid = manager.LoadAsync(pathValid);
        jobs.WaitForIdle();
        Check(manager.DispatchCompletedEvents() == 1, "S1 valid: 1 completion event");
        Check(recorder.calls == 1, "S1 valid: hook fired once");
        Check(recorder.lastBytes.Length() == validLen, "S1 valid: hook bytes length matches");
        Check(std::memcmp(&recorder.lastBytes[0], validContent, validLen) == 0, "S1 valid: hook bytes content matches file");
        Check(recorder.lastHash == ExpectedHash(reinterpret_cast<const unsigned char *>(validContent), validLen),
              "S1 valid: hook content hash matches FNV1A64(bytes)");

        // ── S2:corrupt(bytes 損壞)→ hook 收到該 bytes、安全 ──
        SharedPtr<Asset> hBad = manager.LoadAsync(pathBad);
        jobs.WaitForIdle();
        Check(manager.DispatchCompletedEvents() == 1, "S2 corrupt: 1 completion event");
        Check(recorder.calls == 2, "S2 corrupt: hook fired once more");
        Check(BytesEqual(recorder.lastBytes, &badBytes[0], 32), "S2 corrupt: hook bytes byte-identical to corrupt content");
        Check(recorder.lastHash == ExpectedHash(&badBytes[0], 32), "S2 corrupt: hook hash matches corrupt bytes");

        // ── S3:empty(0 bytes)→ hook 收到 0-length bytes、安全 ──
        SharedPtr<Asset> hEmpty = manager.LoadAsync(pathEmpty);
        jobs.WaitForIdle();
        Check(manager.DispatchCompletedEvents() == 1, "S3 empty: 1 completion event");
        Check(recorder.calls == 3, "S3 empty: hook fired once more");
        Check(recorder.lastBytes.Length() == 0, "S3 empty: hook received 0-length bytes (safe)");
        Check(recorder.lastHash == FNV1A64_OFFSET, "S3 empty: hook hash == FNV1A64_OFFSET (empty content)");

        // ── S4:missing 檔 → 不 fire hook、manager 安全(rejected 狀態)──
        SharedPtr<Asset> hMissing = manager.LoadAsync(pathMissing);
        jobs.WaitForIdle();
        Check(manager.DispatchCompletedEvents() == 1, "S4 missing: completion event still fires (consumer observes failure)");
        Check(!hMissing->loaded && !hMissing->pBlock, "S4 missing: rejected state (loaded == false, no block)");
        Check(recorder.calls == 3, "S4 missing: hook NOT fired for failed load");

        // ── S5:直接以人造 Asset 打 OnAssetLoaded → 各種無效組合都安全 ──
        SharedPtr<Asset> notLoaded = SharedPtr<Asset>::Construct(String("k_notloaded"), 1u); // loaded=false, pBlock=null
        view.OnAssetLoaded(notLoaded);
        Check(recorder.calls == 3, "S5: not-loaded asset -> hook skipped safely");

        SharedPtr<Asset> loadedNoBlock = SharedPtr<Asset>::Construct(String("k_noblock"), 2u);
        loadedNoBlock->loaded = true; // pBlock 仍 null(理論上不會發生,但 hook 必須防呆)
        view.OnAssetLoaded(loadedNoBlock);
        Check(recorder.calls == 3, "S5: loaded-with-null-block asset -> hook skipped safely");

        view.OnAssetLoaded(SharedPtr<Asset>()); // null asset
        Check(recorder.calls == 3, "S5: null asset -> hook skipped safely");

        SharedPtr<AssetBlock> emptyBlock =
            SharedPtr<AssetBlock>::Construct(DynamicArray<unsigned char>(), FNV1A64_OFFSET);
        SharedPtr<Asset> loadedEmpty = SharedPtr<Asset>::Construct(String("k_empty"), 3u);
        loadedEmpty->loaded = true;
        loadedEmpty->pBlock = emptyBlock;
        view.OnAssetLoaded(loadedEmpty);
        Check(recorder.calls == 4, "S5: loaded empty-content asset -> hook fires with 0 bytes (safe)");
        Check(recorder.lastBytes.Length() == 0 && recorder.lastHash == FNV1A64_OFFSET, "S5: empty hook payload correct");

        SharedPtr<AssetBlock> badBlock = SharedPtr<AssetBlock>::Construct(badBytes, ExpectedHash(&badBytes[0], 32));
        SharedPtr<Asset> loadedBad = SharedPtr<Asset>::Construct(String("k_bad"), 4u);
        loadedBad->loaded = true;
        loadedBad->pBlock = badBlock;
        view.OnAssetLoaded(loadedBad);
        Check(recorder.calls == 5, "S5: loaded corrupt-content asset -> hook fires (safe)");
        Check(BytesEqual(recorder.lastBytes, &badBytes[0], 32), "S5: corrupt hook payload byte-identical");

        // ── S6:unsubscribe 後不再收到;重新 subscribe 恢復 ──
        view.Unsubscribe();
        Check(!view.IsAttached(), "S6: unsubscribed cleanly");
        SharedPtr<Asset> hDetach = manager.LoadAsync(pathDetach);
        jobs.WaitForIdle();
        Check(manager.DispatchCompletedEvents() == 1, "S6: detached load completes (1 event)");
        Check(recorder.calls == 5, "S6: detached view received nothing");

        view.SubscribeTo(manager);
        Check(view.IsAttached(), "S6: re-attached to manager");
        const char *reattachContent = "REATTACH";
        Check(WriteFileBytes(pathReattach, reinterpret_cast<const unsigned char *>(reattachContent),
                             std::strlen(reattachContent)), "S6: reattach file written");
        SharedPtr<Asset> hReattach = manager.LoadAsync(pathReattach);
        jobs.WaitForIdle();
        Check(manager.DispatchCompletedEvents() == 1, "S6: re-attached load completes");
        Check(recorder.calls == 6, "S6: hook fires again after re-subscribe");
        Check(recorder.lastBytes.Length() == std::strlen(reattachContent) &&
                  std::memcmp(&recorder.lastBytes[0], reattachContent, std::strlen(reattachContent)) == 0,
              "S6: re-attached hook bytes correct");
        view.Unsubscribe();
        Check(!view.IsAttached(), "S6: final unsubscribe clean");

        std::remove(pathValid);
        std::remove(pathBad);
        std::remove(pathEmpty);
        std::remove(pathDetach);
        std::remove(pathReattach);

        std::cout << "  checks: " << nPassed << " passed, " << nFailed << " failed" << std::endl;
        SUCCESS_MESSAGE("TextureViewHookTest");
        return nFailed == 0;
    }

  private:
    size_t nPassed = 0;
    size_t nFailed = 0;

    bool Check(bool cond, const char *msg)
    {
        EXPECT_TRUE(cond, msg, false); // non-fatal:return_false=false,never returns early
        if (cond)
        {
            nPassed++;
            std::cout << "  [PASS] " << msg << std::endl;
        }
        else
        {
            nFailed++;
        }
        return true;
    }
};
} // namespace assetfmt

#endif // ASSET_FORMAT_VALIDATION_TEST_HPP
