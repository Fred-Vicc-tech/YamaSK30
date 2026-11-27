// YamaSK30.cpp - VST2-instrumentet: kopplar ljudmotorn (YamaSK30DSP.h) till
// Ableton Live via VST2-gränssnittet.
//
// Etapp 1: inget eget fönster ännu. Live visar parametrarna som reglage
// (klicka på den lilla triangeln i pluginets titelrad).
//
// Trådar: ljudtråden anropar processReplacing/processEvents. Där sker ingen
// allokering och inga lås. Parametrar skrivs/läses som atomiska värden.
#include "vst2.h"
#include "YamaSK30DSP.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <new>
#include <xmmintrin.h>

using namespace yamask30;

namespace {

const VstInt32 kUniqueId = ('Y' << 24) | ('S' << 16) | ('3' << 8) | '0';
const VstInt32 kVersion = 100;            // 0.1.0 (etapp 1)
const int kMaxQueuedEvents = 1024;

void copyString(char* dst, const char* src, size_t maxLen)
{
    size_t i = 0;
    for (; i + 1 < maxLen && src[i]; ++i) dst[i] = src[i];
    dst[i] = 0;
}

// ---------------------------------------------------------------------------
// Fabriksljud (visas i Lives presetlista)
// ---------------------------------------------------------------------------
struct FactoryPreset { const char* name; float v[kNumParams]; };

// Ordning: Volume, Str1, Str2, Str3, SlowAtk, Sustain, SusTime, Mode, Detune,
//          VibDly, VibSpd, VibDep, Brill, Ensembl, Tune
#define SUS(s) (float)(std::log((s) / 0.03) / std::log(1.6 / 0.03))
#define VOL(db) (float)(((db) + 48.0) / 54.0)
#define DET(c) (float)(((c) + 20.0) / 47.0)
const FactoryPreset kPresets[] = {
    { "Dream Strings",   { VOL(0), 1, 1, 0, 1, 1, SUS(0.9), 1, DET(7),  0.25f, 0.25f, 0.20f, 0.5f, 1, 0.4901961f } },
    { "Ensemble 8'",     { VOL(0), 0, 1, 0, 1, 1, SUS(0.6), 0, DET(0),  0.0f,  0.25f, 0.0f,  0.5f, 1, 0.4901961f } },
    { "Full Strings",    { VOL(-3),1, 1, 1, 1, 1, SUS(1.2), 1, DET(10), 0.30f, 0.30f, 0.20f, 0.6f, 1, 0.4901961f } },
    { "Cello 16'",       { VOL(0), 1, 0, 0, 1, 1, SUS(0.7), 1, DET(5),  0.40f, 0.20f, 0.25f, 0.4f, 1, 0.4901961f } },
    { "Bright Violins",  { VOL(0), 0, 1, 1, 1, 1, SUS(0.8), 1, DET(8),  0.25f, 0.35f, 0.25f, 0.8f, 1, 0.4901961f } },
    { "Dry Organ Keyer", { VOL(0), 1, 1, 0, 0, 0, SUS(0.1), 0, DET(0),  0.0f,  0.25f, 0.0f,  0.5f, 0, 0.4901961f } },
};
#undef SUS
#undef VOL
#undef DET
const int kNumPresets = (int)(sizeof kPresets / sizeof kPresets[0]);

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

struct Plugin
{
    AEffect effect;
    audioMasterCallback host = nullptr;
    Engine engine;
    std::atomic<float> params[kNumParams];

    struct MidiMsg { VstInt32 delta; uint8_t status, d1, d2; };
    MidiMsg queue[kMaxQueuedEvents];
    int queueCount = 0;

    StateChunk chunk;
    int program = 0;
    char presetName[32] = "Dream Strings";

    void setParam(int index, float value)
    {
        if (index < 0 || index >= kNumParams) return;
        if (!(value >= 0.0f && value <= 1.0f)) return; // NaN / utanför område
        params[index].store(value);
    }

    void loadPreset(int index)
    {
        if (index < 0 || index >= kNumPresets) return;
        for (int i = 0; i < kNumParams; ++i) setParam(i, kPresets[index].v[i]);
        program = index;
        copyString(presetName, kPresets[index].name, sizeof presetName);
    }
};

Plugin* fromEffect(AEffect* e) { return e ? static_cast<Plugin*>(e->object) : nullptr; }

// ---------------------------------------------------------------------------
// Ljudbehandling
// ---------------------------------------------------------------------------
void processBlock(Plugin* p, float** outputs, VstInt32 n, bool accumulate)
{
    if (!p || !outputs || n <= 0) return;

    const unsigned oldCsr = _mm_getcsr();
    _mm_setcsr(oldCsr | 0x8040); // FTZ + DAZ: inga långsamma denormala tal

    Engine& e = p->engine;
    e.setVolumeNorm(p->params[kParamVolume].load());
    e.setStringNorm(0, p->params[kParamString1].load());
    e.setStringNorm(1, p->params[kParamString2].load());
    e.setStringNorm(2, p->params[kParamString3].load());
    e.setSlowAttackNorm(p->params[kParamSlowAttack].load());
    e.setSustainNorm(p->params[kParamSustain].load());
    e.setSustainTimeNorm(p->params[kParamSustainTime].load());
    e.setModeNorm(p->params[kParamMode].load());
    e.setDetuneNorm(p->params[kParamDetune].load());
    e.setVibDelayNorm(p->params[kParamVibDelay].load());
    e.setVibSpeedNorm(p->params[kParamVibSpeed].load());
    e.setVibDepthNorm(p->params[kParamVibDepth].load());
    e.setBrillianceNorm(p->params[kParamBrilliance].load());
    e.setEnsembleNorm(p->params[kParamEnsemble].load());
    e.setTuneNorm(p->params[kParamTune].load());

    float* left = outputs[0];
    float* right = outputs[1];
    int ev = 0;
    for (VstInt32 i = 0; i < n; ++i) {
        while (ev < p->queueCount && p->queue[ev].delta <= i) {
            const Plugin::MidiMsg& m = p->queue[ev++];
            e.handleMidi(m.status, m.d1, m.d2);
        }
        const float s = (float)e.next(); // SK30 är mono: samma signal i båda kanalerna
        if (accumulate) { left[i] += s; if (right) right[i] += s; }
        else            { left[i] = s;  if (right) right[i] = s; }
    }
    while (ev < p->queueCount) {
        const Plugin::MidiMsg& m = p->queue[ev++];
        e.handleMidi(m.status, m.d1, m.d2);
    }
    p->queueCount = 0;

    _mm_setcsr(oldCsr);
}

void VSTCALLBACK_processReplacing(AEffect* e, float** in, float** out, VstInt32 n) { (void)in; processBlock(fromEffect(e), out, n, false); }
void VSTCALLBACK_process(AEffect* e, float** in, float** out, VstInt32 n) { (void)in; processBlock(fromEffect(e), out, n, true); }

void processEvents(Plugin* p, VstEvents* events)
{
    if (!events) return;
    for (VstInt32 i = 0; i < events->numEvents; ++i) {
        VstEvent* ev = events->events[i];
        if (!ev || ev->type != kVstMidiType) continue;
        if (p->queueCount >= kMaxQueuedEvents) break;
        VstMidiEvent* me = reinterpret_cast<VstMidiEvent*>(ev);
        Plugin::MidiMsg m;
        m.delta = me->deltaFrames < 0 ? 0 : me->deltaFrames;
        m.status = (uint8_t)me->midiData[0];
        m.d1 = (uint8_t)me->midiData[1];
        m.d2 = (uint8_t)me->midiData[2];
        int pos = p->queueCount;
        while (pos > 0 && p->queue[pos - 1].delta > m.delta) { p->queue[pos] = p->queue[pos - 1]; --pos; }
        p->queue[pos] = m;
        ++p->queueCount;
    }
}

void setParameter(AEffect* e, VstInt32 index, float value) { if (Plugin* p = fromEffect(e)) p->setParam(index, value); }

float getParameter(AEffect* e, VstInt32 index)
{
    Plugin* p = fromEffect(e);
    if (!p || index < 0 || index >= kNumParams) return 0.0f;
    return p->params[index].load();
}

VstIntPtr dispatcher(AEffect* e, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
    Plugin* p = fromEffect(e);
    if (!p) return 0;

    switch (opcode) {
    case effOpen: return 0;
    case effClose: delete p; return 0;

    case effSetSampleRate: p->engine.prepare(opt); return 0;
    case effSetBlockSize: return 0;
    case effMainsChanged:
        if (value) p->engine.reset();
        p->queueCount = 0;
        return 0;

    case effProcessEvents: processEvents(p, static_cast<VstEvents*>(ptr)); return 1;

    case effGetParamName:
        if (ptr && index >= 0 && index < kNumParams) copyString((char*)ptr, kParamNames[index], 8);
        return 0;
    case effGetParamLabel:
        if (ptr && index >= 0 && index < kNumParams) copyString((char*)ptr, kParamLabels[index], 8);
        return 0;
    case effGetParamDisplay:
        if (ptr && index >= 0 && index < kNumParams) {
            char buf[32];
            paramDisplayText(index, p->params[index].load(), buf, sizeof buf);
            copyString((char*)ptr, buf, 8);
        }
        return 0;
    case effCanBeAutomated: return (index >= 0 && index < kNumParams) ? 1 : 0;

    case effGetProgram: return p->program < 0 ? 0 : p->program;
    case effSetProgram: p->loadPreset((int)value); return 0;
    case effGetProgramName: if (ptr) copyString((char*)ptr, p->presetName, 24); return 0;
    case effSetProgramName: if (ptr) copyString(p->presetName, (const char*)ptr, sizeof p->presetName); return 0;
    case effGetProgramNameIndexed:
        if (ptr && index >= 0 && index < kNumPresets) { copyString((char*)ptr, kPresets[index].name, 24); return 1; }
        return 0;

    case effGetChunk: {
        if (!ptr) return 0;
        std::memset(&p->chunk, 0, sizeof p->chunk);
        std::memcpy(p->chunk.magic, "YSst", 4);
        p->chunk.version = 1;
        p->chunk.numParams = kNumParams;
        p->chunk.program = p->program;
        copyString(p->chunk.name, p->presetName, sizeof p->chunk.name);
        for (int i = 0; i < kNumParams; ++i) p->chunk.values[i] = p->params[i].load();
        *static_cast<void**>(ptr) = &p->chunk;
        return (VstIntPtr)sizeof(StateChunk);
    }
    case effSetChunk: {
        if (!ptr || value < (VstIntPtr)kChunkHeader) return 0;
        const unsigned char* data = static_cast<const unsigned char*>(ptr);
        if (std::memcmp(data, "YSst", 4) != 0) return 0;
        uint32_t count = 0;
        std::memcpy(&count, data + 8, 4);
        int32_t prog = 0;
        std::memcpy(&prog, data + 12, 4);
        p->program = (prog >= -1 && prog < kNumPresets) ? prog : -1;
        char nm[32];
        std::memcpy(nm, data + 16, 32);
        nm[31] = 0;
        copyString(p->presetName, nm, sizeof p->presetName);
        for (int i = 0; i < kNumParams; ++i) p->setParam(i, defaultParam(i)); // nya parametrar får standardvärden
        const size_t available = ((size_t)value - kChunkHeader) / sizeof(float);
        if (count > available) count = (uint32_t)available;
        for (uint32_t i = 0; i < count && i < (uint32_t)kNumParams; ++i) {
            float v;
            std::memcpy(&v, data + kChunkHeader + i * sizeof(float), sizeof(float));
            p->setParam((int)i, v);
        }
        return 1;
    }

    case effGetPlugCategory: return kPlugCategSynth;
    case effGetEffectName: if (ptr) copyString((char*)ptr, "YamaSK30", 32); return 1;
    case effGetProductString: if (ptr) copyString((char*)ptr, "YamaSK30 Symphonic Ensemble", 64); return 1;
    case effGetVendorString: if (ptr) copyString((char*)ptr, "2F Productions", 64); return 1;
    case effGetVendorVersion: return kVersion;
    case effGetVstVersion: return 2400;
    case effGetTailSize: return 0;

    case effCanDo: {
        const char* s = static_cast<const char*>(ptr);
        if (!s) return 0;
        if (!std::strcmp(s, "receiveVstEvents") || !std::strcmp(s, "receiveVstMidiEvent")) return 1;
        return 0;
    }
    default: return 0;
    }
}

} // namespace

VST_EXPORT AEffect* VSTPluginMain(audioMasterCallback host)
{
    if (!host) return nullptr;
    if (host(nullptr, audioMasterVersion, 0, 0, nullptr, 0.0f) == 0) return nullptr;

    Plugin* p = new (std::nothrow) Plugin();
    if (!p) return nullptr;
    p->host = host;
    for (int i = 0; i < kNumParams; ++i) p->params[i].store(defaultParam(i));
    p->loadPreset(0);

    AEffect& e = p->effect;
    std::memset(&e, 0, sizeof(AEffect));
    e.magic = kEffectMagic;
    e.dispatcher = dispatcher;
    e.process = VSTCALLBACK_process;
    e.processReplacing = VSTCALLBACK_processReplacing;
    e.setParameter = setParameter;
    e.getParameter = getParameter;
    e.numPrograms = kNumPresets;
    e.numParams = kNumParams;
    e.numInputs = 0;
    e.numOutputs = 2;
    e.flags = effFlagsCanReplacing | effFlagsIsSynth | effFlagsProgramChunks;
    e.object = p;
    e.uniqueID = kUniqueId;
    e.version = kVersion;

    p->engine.prepare(44100.0);
    return &e;
}
