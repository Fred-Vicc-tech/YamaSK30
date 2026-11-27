// TrueSymph.cpp - VST2-instrumentet: kopplar kärnan (TrueSymphCore.h) till
// Ableton Live via VST2-gränssnittet. Fönstret finns i TrueSymphEditor.cpp.
//
// Trådar: ljudtråden anropar processReplacing/processEvents. Där sker ingen
// allokering och inga lås. Parametrar skrivs/läses som atomiska värden.
#include "vst2.h"
#include "TrueSymphCore.h"

#include <cstdio>
#include <cstring>
#include <new>

#if defined(_WIN32)
  #define TRUESYMPH_HAS_EDITOR 1
#else
  #define TRUESYMPH_HAS_EDITOR 0
#endif

using namespace truesymph;

namespace {

// Samma ID som när pluginet hette YamaSK30, så att sparade Live-projekt fortsätter fungera.
const VstInt32 kUniqueId = ('Y' << 24) | ('S' << 16) | ('3' << 8) | '0';
const VstInt32 kVersion = 800;            // 0.7.0 (effekter: delay + reverb, wah-bas)

struct Plugin : Core
{
    AEffect effect;
    audioMasterCallback host = nullptr;
    ERect rect;

    void hostAutomate(int index, float value) override { if (host) host(&effect, audioMasterAutomate, index, 0, nullptr, value); }
    void hostBeginEdit(int index) override { if (host) host(&effect, audioMasterBeginEdit, index, 0, nullptr, 0.0f); }
    void hostEndEdit(int index) override { if (host) host(&effect, audioMasterEndEdit, index, 0, nullptr, 0.0f); }
    void hostUpdateDisplay() override { if (host) host(&effect, audioMasterUpdateDisplay, 0, 0, nullptr, 0.0f); }
};

Plugin* fromEffect(AEffect* e) { return e ? static_cast<Plugin*>(e->object) : nullptr; }

void VSTCALLBACK_processReplacing(AEffect* e, float** in, float** out, VstInt32 n)
{
    (void)in;
    Plugin* p = fromEffect(e);
    if (p && out) p->processAudio(out[0], out[1], n, false);
}
void VSTCALLBACK_process(AEffect* e, float** in, float** out, VstInt32 n)
{
    (void)in;
    Plugin* p = fromEffect(e);
    if (p && out) p->processAudio(out[0], out[1], n, true);
}

void processEvents(Plugin* p, VstEvents* events)
{
    if (!events) return;
    for (VstInt32 i = 0; i < events->numEvents; ++i) {
        VstEvent* ev = events->events[i];
        if (!ev || ev->type != kVstMidiType) continue;
        VstMidiEvent* me = reinterpret_cast<VstMidiEvent*>(ev);
        p->queueMidi(me->deltaFrames, (uint8_t)me->midiData[0], (uint8_t)me->midiData[1], (uint8_t)me->midiData[2]);
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
    case effClose:
#if TRUESYMPH_HAS_EDITOR
        editorClose(p);
#endif
        delete p;
        return 0;

    case effSetSampleRate: p->engine.prepare(opt); e->initialDelay = p->engine.latencySamples(); return 0;
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
    case effSetProgram: p->loadFactoryPreset((int)value, false); return 0;
    case effGetProgramName: if (ptr) copyString((char*)ptr, p->presetName, 24); return 0;
    case effSetProgramName: if (ptr) copyString(p->presetName, cleanPresetName((const char*)ptr).c_str(), sizeof p->presetName); return 0;
    case effGetProgramNameIndexed:
        if (ptr && index >= 0 && index < kNumFactoryPresets) { copyString((char*)ptr, factoryPresetName(index), 24); return 1; }
        return 0;

    case effGetChunk: {
        if (!ptr) return 0;
        *static_cast<const void**>(ptr) = &p->saveState();
        return (VstIntPtr)sizeof(StateChunk);
    }
    case effSetChunk: return (ptr && value > 0 && p->loadState(ptr, (size_t)value)) ? 1 : 0;

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

#if TRUESYMPH_HAS_EDITOR
    case effEditGetRect:
        if (ptr) *static_cast<ERect**>(ptr) = &p->rect;
        return 1;
    case effEditOpen: editorOpen(p, ptr); return 1;
    case effEditClose: editorClose(p); return 1;
    case effEditIdle: return 0;
#endif
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

    AEffect& e = p->effect;
    std::memset(&e, 0, sizeof(AEffect));
    e.magic = kEffectMagic;
    e.dispatcher = dispatcher;
    e.process = VSTCALLBACK_process;
    e.processReplacing = VSTCALLBACK_processReplacing;
    e.setParameter = setParameter;
    e.getParameter = getParameter;
    e.numPrograms = kNumFactoryPresets;
    e.numParams = kNumParams;
    e.numInputs = 0;
    e.numOutputs = 2;
    e.flags = effFlagsCanReplacing | effFlagsIsSynth | effFlagsProgramChunks
#if TRUESYMPH_HAS_EDITOR
            | effFlagsHasEditor
#endif
            ;
    e.object = p;
    e.uniqueID = kUniqueId;
    e.version = kVersion;

    p->rect.top = 0; p->rect.left = 0;
    p->rect.bottom = (int16_t)kEditorHeight; p->rect.right = (int16_t)kEditorWidth;

    p->engine.prepare(44100.0);
    e.initialDelay = p->engine.latencySamples();   // limiterns framförhållning (Live kompenserar)
    return &e;
}
