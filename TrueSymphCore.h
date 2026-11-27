// TrueSymphCore.h - kärnan som VST2-delen och fönstret delar: parametrar
// (atomiska, delas mellan ljudtråd och fönster), MIDI-kö, ljudbehandling per
// block, sparat tillstånd (chunk) och fabriksljud. Värdprogrammet kopplas in
// via de virtuella host*-funktionerna.
#pragma once

#include "TrueSymphDSP.h"
#include "Presets.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#if defined(_M_X64) || defined(__x86_64__) || defined(_M_IX86) || defined(__i386__)
  #include <xmmintrin.h>
  #define TRUESYMPH_HAS_SSE 1
#else
  #define TRUESYMPH_HAS_SSE 0
#endif

namespace truesymph {

const int kEditorWidth = 1180, kEditorHeight = 668;
const uint32_t kStateVersion = 1;
const int kMaxQueuedEvents = 1024;

inline void copyString(char* dst, const char* src, size_t maxLen)
{
    size_t i = 0;
    for (; i + 1 < maxLen && src[i]; ++i) dst[i] = src[i];
    dst[i] = 0;
}

// Sparat tillstånd ("chunk") som Live lagrar i projektet. Samma format sedan
// version 0.1 (magic "YSst" från tiden som YamaSK30), så gamla projekt öppnas.
#pragma pack(push, 1)
struct StateChunk
{
    char magic[4];          // "YSst"
    uint32_t version;       // 1
    uint32_t numParams;
    int32_t program;        // vald fabrikspreset (-1 = egen)
    char name[32];
    float values[kNumParams];
};
#pragma pack(pop)
const size_t kChunkHeader = 48;

struct Core
{
    Engine engine;
    std::atomic<float> params[kNumParams];

    // MIDI-kö: fylls i processEvents, töms i processAudio (samma tråd)
    struct MidiMsg { int delta; uint8_t status, d1, d2; };
    MidiMsg queue[kMaxQueuedEvents];
    int queueCount = 0;

    // För fönstret
    std::atomic<float> peak{ 0.0f };          // högsta nivå sedan fönstret läste
    std::atomic<int> soloNote{ -1 };
    std::atomic<float> limitDb{ 0.0f };       // limiterns största sänkning sedan fönstret läste

    StateChunk chunk;
    int program = 0;
    char presetName[32] = "Dream Strings";
    void* editor = nullptr;                   // fönstret (TrueSymphEditor.cpp), om öppet

    Core()
    {
        for (int i = 0; i < kNumParams; ++i) params[i].store(defaultParam(i));
        loadFactoryPreset(0, false);
    }
    virtual ~Core() {}

    // --- Kopplingar till värdprogrammet ---
    virtual void hostAutomate(int /*index*/, float /*value*/) {}
    virtual void hostBeginEdit(int /*index*/) {}
    virtual void hostEndEdit(int /*index*/) {}
    virtual void hostUpdateDisplay() {}

    void setParam(int index, float value)
    {
        if (index < 0 || index >= kNumParams) return;
        if (!(value >= 0.0f && value <= 1.0f)) return; // NaN / utanför område
        params[index].store(value);
    }
    void setParamFromEditor(int index, float value)
    {
        setParam(index, value);
        if (index >= 0 && index < kNumParams) hostAutomate(index, params[index].load());
    }
    void beginEdit(int index) { hostBeginEdit(index); }
    void endEdit(int index) { hostEndEdit(index); }

    void applyValues(const float* v, bool fromEditor)
    {
        for (int i = 0; i < kNumParams; ++i) {
            if (fromEditor) { beginEdit(i); setParamFromEditor(i, v[i]); endEdit(i); }
            else setParam(i, v[i]);
        }
    }
    void loadFactoryPreset(int index, bool fromEditor)
    {
        if (index < 0 || index >= kNumFactoryPresets) return;
        float v[kNumParams];
        factoryPresetValues(index, v);
        applyValues(v, fromEditor);
        program = index;
        copyString(presetName, factoryPresetName(index), sizeof presetName);
    }

    // --- MIDI ---
    void queueMidi(int delta, uint8_t status, uint8_t d1, uint8_t d2)
    {
        if (queueCount >= kMaxQueuedEvents) return;
        MidiMsg m; m.delta = delta < 0 ? 0 : delta; m.status = status; m.d1 = d1; m.d2 = d2;
        int pos = queueCount;                       // sortera in efter tid
        while (pos > 0 && queue[pos - 1].delta > m.delta) { queue[pos] = queue[pos - 1]; --pos; }
        queue[pos] = m;
        ++queueCount;
    }

    // --- Ljudbehandling (ljudtråden). Mono som SK30, stereo när delay/reverb är på.
    void processAudio(float* outL, float* outR, int n, bool accumulate)
    {
        if (!outL || n <= 0) return;
#if TRUESYMPH_HAS_SSE
        const unsigned oldCsr = _mm_getcsr();
        _mm_setcsr(oldCsr | 0x8040); // FTZ + DAZ: inga långsamma denormala tal
#endif
        float v[kNumParams];
        for (int i = 0; i < kNumParams; ++i) v[i] = params[i].load();
        engine.setParams(v);
        int ev = 0;
        double pk = 0.0;
        for (int i = 0; i < n; ++i) {
            while (ev < queueCount && queue[ev].delta <= i) { const MidiMsg& m = queue[ev++]; engine.handleMidi(m.status, m.d1, m.d2); }
            double l, r;
            engine.nextStereo(l, r);
            if (accumulate) { outL[i] += (float)l; if (outR) outR[i] += (float)r; }
            else            { outL[i] = (float)l;  if (outR) outR[i] = (float)r; }
            const double a = std::fmax(std::fabs(l), std::fabs(r)); if (a > pk) pk = a;
        }
        while (ev < queueCount) { const MidiMsg& m = queue[ev++]; engine.handleMidi(m.status, m.d1, m.d2); }
        queueCount = 0;
        if ((float)pk > peak.load()) peak.store((float)pk);
        soloNote.store(engine.soloCurrentNote());
        { const float r = (float)engine.takeLimiterReductionDb(); if (r > limitDb.load()) limitDb.store(r); }
#if TRUESYMPH_HAS_SSE
        _mm_setcsr(oldCsr);
#endif
    }

    // --- Sparat tillstånd ---
    const StateChunk& saveState()
    {
        std::memset(&chunk, 0, sizeof chunk);
        std::memcpy(chunk.magic, "YSst", 4);
        chunk.version = kStateVersion;
        chunk.numParams = kNumParams;
        chunk.program = program;
        copyString(chunk.name, presetName, sizeof chunk.name);
        for (int i = 0; i < kNumParams; ++i) chunk.values[i] = params[i].load();
        return chunk;
    }
    bool loadState(const void* ptr, size_t size)
    {
        if (!ptr || size < kChunkHeader) return false;
        const unsigned char* data = static_cast<const unsigned char*>(ptr);
        if (std::memcmp(data, "YSst", 4) != 0) return false;
        uint32_t count = 0;
        std::memcpy(&count, data + 8, 4);
        int32_t prog = 0;
        std::memcpy(&prog, data + 12, 4);
        program = (prog >= -1 && prog < kNumFactoryPresets) ? prog : -1;
        char nm[33];
        std::memcpy(nm, data + 16, 32);
        nm[32] = 0;
        copyString(presetName, cleanPresetName(nm).c_str(), sizeof presetName);
        for (int i = 0; i < kNumParams; ++i) setParam(i, defaultParam(i)); // nya parametrar får standardvärden
        const size_t available = (size - kChunkHeader) / sizeof(float);
        if (count > available) count = (uint32_t)available;
        for (uint32_t i = 0; i < count && i < (uint32_t)kNumParams; ++i) {
            float v;
            std::memcpy(&v, data + kChunkHeader + i * sizeof(float), sizeof(float));
            setParam((int)i, v);
        }
        return true;
    }
};

// Fönstret (Windows). Öppnas i ett föräldrafönster från värdprogrammet.
void editorOpen(Core* core, void* parentWindow);
void editorClose(Core* core);
// Ritar fönstret till en PNG-fil (bilder av gränssnittet i bygget)
bool editorRenderPng(Core* core, bool backSide, const wchar_t* path);

} // namespace truesymph
