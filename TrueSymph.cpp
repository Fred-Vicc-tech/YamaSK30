// TrueSymph.cpp - VST2-instrumentet: kopplar ljudmotorn (TrueSymphDSP.h) till
// Ableton Live via VST2-gränssnittet.
//
// Inget eget fönster ännu. Live visar parametrarna som reglage
// (klicka på den lilla triangeln i pluginets titelrad).
//
// Trådar: ljudtråden anropar processReplacing/processEvents. Där sker ingen
// allokering och inga lås. Parametrar skrivs/läses som atomiska värden.
#include "vst2.h"
#include "TrueSymphDSP.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <new>
#include <xmmintrin.h>

using namespace truesymph;

namespace {

// Samma ID som när pluginet hette YamaSK30, så att sparade Live-projekt fortsätter fungera.
const VstInt32 kUniqueId = ('Y' << 24) | ('S' << 16) | ('3' << 8) | '0';
const VstInt32 kVersion = 400;            // 0.4.0 (etapp 4: solosynt, manual bass, split)
const int kMaxQueuedEvents = 1024;

void copyString(char* dst, const char* src, size_t maxLen)
{
    size_t i = 0;
    for (; i + 1 < maxLen && src[i]; ++i) dst[i] = src[i];
    dst[i] = 0;
}

// ---------------------------------------------------------------------------
// Fabriksljud (visas i Lives presetlista). Varje ljud utgår från
// standardvärdena och ändrar bara de parametrar som räknas upp.
// ---------------------------------------------------------------------------
struct PresetValue { int index; float value; };
struct FactoryPreset { const char* name; PresetValue v[20]; int count; };

inline float SUS(double s) { return (float)(std::log(s / 0.03) / std::log(1.6 / 0.03)); }
inline float VOL(double db) { return (float)normFromVolumeDb(db); }
inline float DET(double c) { return (float)normFromDetune(c); }
inline float SEL(int i) { return (float)stepNorm(i, kNumPolySel); }
inline float FEET(int i) { return (float)stepNorm(i, kNumFeet); }
inline float OSEL(int i) { return (float)stepNorm(i, kNumOrgSel); }
inline float LEV(double v) { return (float)(v / 10.0); }   // tone lever 0..10
inline float SMODE(int i) { return (float)stepNorm(i, kNumSoloModes); }
inline float SFEET(int i) { return (float)stepNorm(i, kNumSoloFeet); }
inline float VCA(int i) { return (float)stepNorm(i, kNumVcaModes); }
inline float PWS(int i) { return (float)stepNorm(i, kNumPwSources); }
inline float SPLIT(int i) { return (float)stepNorm(i, kNumSplit); }

const FactoryPreset kPresets[] = {
    { "Dream Strings",   { }, 0 },
    { "Ensemble 8'",     { {kParamString1, 0}, {kParamSustainTime, SUS(0.6)}, {kParamMode, 0}, {kParamDetune, DET(0)},
                           {kParamVibDelay, 0}, {kParamVibDepth, 0} }, 6 },
    { "Full Strings",    { {kParamVolume, VOL(-3)}, {kParamString3, 1}, {kParamSustainTime, SUS(1.2)}, {kParamDetune, DET(10)},
                           {kParamVibDelay, 0.3f}, {kParamVibSpeed, 0.3f}, {kParamBrilliance, 0.6f} }, 7 },
    { "Cello 16'",       { {kParamString2, 0}, {kParamSustainTime, SUS(0.7)}, {kParamDetune, DET(5)}, {kParamVibDelay, 0.4f},
                           {kParamVibSpeed, 0.2f}, {kParamVibDepth, 0.25f}, {kParamBrilliance, 0.4f} }, 7 },
    { "Bright Violins",  { {kParamString1, 0}, {kParamString3, 1}, {kParamSustainTime, SUS(0.8)}, {kParamDetune, DET(8)},
                           {kParamVibSpeed, 0.35f}, {kParamVibDepth, 0.25f}, {kParamBrilliance, 0.8f} }, 7 },
    { "Dry Organ Keyer", { {kParamSlowAttack, 0}, {kParamSustain, 0}, {kParamMode, 0}, {kParamDetune, DET(0)},
                           {kParamVibDelay, 0}, {kParamVibDepth, 0}, {kParamEnsemble, 0} }, 7 },
    { "Poly-Synth 1",    { {kParamString1, 0}, {kParamString2, 0}, {kParamPolySel, SEL(1)} }, 3 },
    { "Poly-Synth 2",    { {kParamString1, 0}, {kParamString2, 0}, {kParamPolySel, SEL(2)} }, 3 },
    { "Poly-Synth 3",    { {kParamString1, 0}, {kParamString2, 0}, {kParamPolySel, SEL(3)} }, 3 },
    { "Strings + Poly 2",{ {kParamPolySel, SEL(2)}, {kParamPolyVol, VOL(-6)} }, 2 },
    { "Soft Pad",        { {kParamString1, 0}, {kParamString2, 0}, {kParamPolySel, SEL(4)}, {kParamPolyFeet, FEET(3)},
                           {kParamCutoff, 0.45f}, {kParamResonance, 0.1f}, {kParamEgDepth, 0.25f}, {kParamAttack, 0.8f},
                           {kParamDecay, 0.7f}, {kParamSustainLevel, 0.6f}, {kParamRelease, 0.6f}, {kParamSustainTime, SUS(1.4)},
                           {kParamPolyVol, VOL(6)} }, 13 },
    { "Wah Brass",       { {kParamString1, 0}, {kParamString2, 0}, {kParamPolySel, SEL(4)}, {kParamPolyFeet, FEET(5)},
                           {kParamCutoff, 0.3f}, {kParamResonance, 0.6f}, {kParamEgDepth, 0.45f}, {kParamAttack, 0.42f},
                           {kParamDecay, 0.45f}, {kParamSustainLevel, 0.45f}, {kParamRelease, 0.3f}, {kParamSlowAttack, 0},
                           {kParamSustainTime, SUS(0.3)}, {kParamEnsemble, 0} }, 14 },
    { "Tremolo Strings", { {kParamEnsemble, 0}, {kParamPolyTremolo, 1}, {kParamTremoloSpeed, 1} }, 3 },
    { "Organ 1",         { {kParamString1, 0}, {kParamString2, 0}, {kParamOrgSel, OSEL(1)} }, 3 },
    { "Organ 2",         { {kParamString1, 0}, {kParamString2, 0}, {kParamOrgSel, OSEL(2)} }, 3 },
    { "Organ 3 Full",    { {kParamString1, 0}, {kParamString2, 0}, {kParamOrgSel, OSEL(3)}, {kParamOrgVol, VOL(-4)} }, 4 },
    { "Jazz Percussion", { {kParamString1, 0}, {kParamString2, 0}, {kParamOrgSel, OSEL(4)},
                           {kParamOrg16, LEV(8)}, {kParamOrg8, LEV(8)}, {kParamOrg4, LEV(0)}, {kParamPerc3, LEV(8)},
                           {kParamPercDecay, 0.35f}, {kParamOrgSustain, 0}, {kParamOrgTremolo, 1}, {kParamTremoloSpeed, 0} , {kParamOrgVol, VOL(4)} }, 12 },
    { "Rotor Organ",     { {kParamString1, 0}, {kParamString2, 0}, {kParamOrgSel, OSEL(4)},
                           {kParamOrg16, LEV(8)}, {kParamOrg8, LEV(8)}, {kParamOrg513, LEV(6)}, {kParamOrg4, LEV(5)},
                           {kParamOrg2, LEV(4)}, {kParamOrgTremolo, 1}, {kParamTremoloSpeed, 1}, {kParamOrgSustain, 0} , {kParamOrgVol, VOL(4)} }, 12 },
    { "Ensemble Organ",  { {kParamString1, 0}, {kParamString2, 0}, {kParamOrgSel, OSEL(4)},
                           {kParamOrg8, LEV(8)}, {kParamOrg4, LEV(6)}, {kParamOrg2, LEV(4)}, {kParamOrg16, LEV(5)},
                           {kParamOrgEnsemble, 1}, {kParamOrgTime, 0.75f} , {kParamOrgVol, VOL(4)} }, 10 },
    { "Organ Piano",     { {kParamString1, 0}, {kParamString2, 0}, {kParamOrgSel, OSEL(4)},
                           {kParamOrg16, LEV(6)}, {kParamOrg8, LEV(9)}, {kParamOrg4, LEV(7)}, {kParamOrg2, LEV(4)},
                           {kParamOrgDecay, 1}, {kParamOrgTime, 0.8f}, {kParamOrgSustain, 1}, {kParamOrgBrill, 0.7f} , {kParamOrgVol, VOL(6)} }, 12 },
    { "Strings + Organ", { {kParamOrgSel, OSEL(4)}, {kParamOrg16, LEV(6)}, {kParamOrg8, LEV(7)}, {kParamOrg4, LEV(4)},
                           {kParamOrgVol, VOL(-4)}, {kParamOrgEnsemble, 1} }, 6 },
    { "Solo Lead",       { {kParamString1, 0}, {kParamString2, 0}, {kParamSoloVol, VOL(0)}, {kParamSoloCutoff, 0.5f},
                           {kParamSoloReso, 0.35f}, {kParamSoloEgDepth, 0.4f}, {kParamPortamento, 0.25f},
                           {kParamTouchVib, 1}, {kParamTouchBrill, 1} }, 9 },
    { "Square Solo PWM", { {kParamString1, 0}, {kParamString2, 0}, {kParamSoloVol, VOL(0)}, {kParamSoloWave, 1},
                           {kParamSoloPw, 0.8f}, {kParamPwSource, PWS(0)}, {kParamLfoSpeed, 0.25f}, {kParamSoloCutoff, 0.6f},
                           {kParamSoloReso, 0.15f}, {kParamTouchVib, 1} }, 10 },
    { "Strings + Man Bass", { {kParamSoloVol, VOL(-4)}, {kParamSoloMode, SMODE(2)}, {kParamSoloFeet, SFEET(3)},
                           {kParamSoloCutoff, 0.42f}, {kParamSoloReso, 0.25f}, {kParamSoloEgDepth, 0.35f}, {kParamSoloDecay, 0.3f},
                           {kParamSoloSustain, 0.4f}, {kParamSoloRelease, 0.15f} }, 9 },
    { "Strings + Bass Mode", { {kParamSoloVol, VOL(-6)}, {kParamSoloMode, SMODE(1)}, {kParamSoloFeet, SFEET(3)},
                           {kParamSoloWave, 0.5f}, {kParamSoloCutoff, 0.38f}, {kParamSoloEgDepth, 0.3f} }, 6 },
    { "Glide Lead",      { {kParamString1, 0}, {kParamString2, 0}, {kParamSoloVol, VOL(0)}, {kParamGlide, 0.3f},
                           {kParamSoloFeet, SFEET(0)}, {kParamSoloCutoff, 0.45f}, {kParamSoloReso, 0.5f}, {kParamSoloEgDepth, 0.5f},
                           {kParamSoloMod, 0.1f}, {kParamLfoSpeed, 0.3f} }, 10 },
    { "Split Poly|Organ", { {kParamSplit, SPLIT(1)}, {kParamString1, 0}, {kParamPolySel, SEL(2)}, {kParamOrgSel, OSEL(2)},
                           {kParamOrgVol, VOL(-4)}, {kParamOrgEnsemble, 1} }, 6 },
    { "Slow Rotor Pad",  { {kParamString3, 1}, {kParamEnsemble, 0}, {kParamPolyTremolo, 1}, {kParamTremoloSpeed, 0},
                           {kParamSustainTime, SUS(1.4)}, {kParamVolume, VOL(-3)} }, 6 },
};
const int kNumPresets = (int)(sizeof kPresets / sizeof kPresets[0]);

void presetValues(int index, float* out)
{
    for (int i = 0; i < kNumParams; ++i) out[i] = defaultParam(i);
    for (int j = 0; j < kPresets[index].count; ++j) {
        const PresetValue& pv = kPresets[index].v[j];
        if (pv.index >= 0 && pv.index < kNumParams) out[pv.index] = pv.value;
    }
}

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
        float v[kNumParams];
        presetValues(index, v);
        for (int i = 0; i < kNumParams; ++i) setParam(i, v[i]);
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
    e.setPolySelNorm(p->params[kParamPolySel].load());
    e.setPolyVolNorm(p->params[kParamPolyVol].load());
    e.setPolyFeetNorm(p->params[kParamPolyFeet].load());
    e.setCutoffNorm(p->params[kParamCutoff].load());
    e.setResonanceNorm(p->params[kParamResonance].load());
    e.setEgDepthNorm(p->params[kParamEgDepth].load());
    e.setAttackNorm(p->params[kParamAttack].load());
    e.setDecayNorm(p->params[kParamDecay].load());
    e.setSustainLevelNorm(p->params[kParamSustainLevel].load());
    e.setReleaseNorm(p->params[kParamRelease].load());
    e.setPolyTremoloNorm(p->params[kParamPolyTremolo].load());
    e.setTremoloSpeedNorm(p->params[kParamTremoloSpeed].load());
    e.setOrgSelNorm(p->params[kParamOrgSel].load());
    e.setOrgVolNorm(p->params[kParamOrgVol].load());
    e.setOrgDecayNorm(p->params[kParamOrgDecay].load());
    e.setOrgSustainNorm(p->params[kParamOrgSustain].load());
    e.setOrgTimeNorm(p->params[kParamOrgTime].load());
    e.setOrgBrillNorm(p->params[kParamOrgBrill].load());
    e.setPercDecayNorm(p->params[kParamPercDecay].load());
    for (int i = 0; i < 3; ++i) e.setPercNorm(i, p->params[kParamPerc2 + i].load());
    for (int i = 0; i < kNumFootages; ++i) e.setFootageNorm(i, p->params[kParamOrg16 + i].load());
    e.setOrgTremoloNorm(p->params[kParamOrgTremolo].load());
    e.setOrgEnsembleNorm(p->params[kParamOrgEnsemble].load());
    e.setMasterNorm(p->params[kParamMaster].load());
    e.setSoloVolNorm(p->params[kParamSoloVol].load());
    e.setSoloModeNorm(p->params[kParamSoloMode].load());
    e.setLfoSpeedNorm(p->params[kParamLfoSpeed].load());
    e.setPortamentoNorm(p->params[kParamPortamento].load());
    e.setGlideNorm(p->params[kParamGlide].load());
    e.setSoloFeetNorm(p->params[kParamSoloFeet].load());
    e.setSoloWaveNorm(p->params[kParamSoloWave].load());
    e.setSoloPwNorm(p->params[kParamSoloPw].load());
    e.setPwSourceNorm(p->params[kParamPwSource].load());
    e.setSoloCutoffNorm(p->params[kParamSoloCutoff].load());
    e.setSoloResoNorm(p->params[kParamSoloReso].load());
    e.setSoloModNorm(p->params[kParamSoloMod].load());
    e.setSoloEgDepthNorm(p->params[kParamSoloEgDepth].load());
    e.setVcaModeNorm(p->params[kParamVcaMode].load());
    e.setSoloAttackNorm(p->params[kParamSoloAttack].load());
    e.setSoloDecayNorm(p->params[kParamSoloDecay].load());
    e.setSoloSustainNorm(p->params[kParamSoloSustain].load());
    e.setSoloReleaseNorm(p->params[kParamSoloRelease].load());
    e.setTouchVibNorm(p->params[kParamTouchVib].load());
    e.setTouchBrillNorm(p->params[kParamTouchBrill].load());
    e.setTouchModNorm(p->params[kParamTouchMod].load());
    e.setSoloTuneNorm(p->params[kParamSoloTune].load());
    e.setSplitNorm(p->params[kParamSplit].load());
    e.setSplitPointNorm(p->params[kParamSplitPoint].load());
    e.setBassPointNorm(p->params[kParamBassPoint].load());

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
    case effGetEffectName: if (ptr) copyString((char*)ptr, "TrueSymph", 32); return 1;
    case effGetProductString: if (ptr) copyString((char*)ptr, "TrueSymph Symphonic Ensemble", 64); return 1;
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
